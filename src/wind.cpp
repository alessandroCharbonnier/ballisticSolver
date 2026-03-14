#include "wind.h"
#include "config.h"
#include <Arduino.h>
#include <cstring>
#include <cstdlib>

// Use UART2
static HardwareSerial& s_uart = Serial2;

void WindSensor::begin() {
    s_uart.begin(cfg::WIND_BAUD, SERIAL_8N1,
                 cfg::PIN_WIND_RX, cfg::PIN_WIND_TX);
}

void WindSensor::poll() {
    while (s_uart.available()) {
        char c = static_cast<char>(s_uart.read());

        if (c == '$') {
            // Start of new sentence
            buf_idx_ = 0;
            buf_[buf_idx_++] = c;
        } else if (c == '\n' || c == '\r') {
            // End of sentence
            if (buf_idx_ > 0) {
                buf_[buf_idx_] = '\0';
                parseNMEA(buf_);
                buf_idx_ = 0;
            }
        } else if (buf_idx_ < sizeof(buf_) - 1) {
            buf_[buf_idx_++] = c;
        }
    }
}

void WindSensor::parseNMEA(const char* sentence) {
    // We only care about $IIMWV (wind speed and angle)
    // Format: $IIMWV,<angle>,<R|T>,<speed>,<unit>,<status>*hh
    if (strncmp(sentence, "$IIMWV", 6) != 0) return;
    if (!verifyChecksum(sentence)) return;

    // Tokenise by comma
    char copy[83];
    strncpy(copy, sentence, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char* tokens[7] = {nullptr};
    int idx = 0;
    char* tok = strtok(copy, ",*");
    while (tok && idx < 7) {
        tokens[idx++] = tok;
        tok = strtok(nullptr, ",*");
    }

    // tokens: [0]=$IIMWV [1]=angle [2]=R/T [3]=speed [4]=unit [5]=status
    if (idx < 6) return;
    if (tokens[5][0] != 'A') return;  // A = valid

    float angle = strtof(tokens[1], nullptr);
    float speed = strtof(tokens[3], nullptr);

    // Convert speed to mph based on unit
    // K=km/h, N=knots, M=m/s, S=mph
    char unit = tokens[4][0];
    switch (unit) {
        case 'K': speed *= 0.621371f; break;   // km/h → mph
        case 'N': speed *= 1.15078f;  break;   // knots → mph
        case 'M': speed *= 2.23694f;  break;   // m/s → mph
        case 'S': break;                        // already mph
        default:  break;
    }

    data_.angle_deg      = angle;
    data_.speed_mph      = speed;
    data_.available      = true;
    data_.last_update_ms = millis();
}

bool WindSensor::verifyChecksum(const char* sentence) {
    // Checksum is XOR of all chars between '$' and '*'
    const char* p = sentence + 1;  // skip '$'
    uint8_t cs = 0;
    while (*p && *p != '*') {
        cs ^= static_cast<uint8_t>(*p);
        ++p;
    }
    if (*p != '*') return false;
    uint8_t expected = static_cast<uint8_t>(strtol(p + 1, nullptr, 16));
    return cs == expected;
}
