#pragma once
/// @file wind.h
/// @brief Calypso ultrasonic anemometer interface (NMEA over UART2).
///
/// Prepared for future integration.  Currently returns zero until
/// the Calypso module is physically connected.
///
/// The Calypso Mini outputs standard NMEA 0183 sentences:
///   $IIMWV,<angle>,<R|T>,<speed>,<unit>,<status>*hh\r\n
///
/// We parse $IIMWV (wind speed and angle).

#include <cstdint>

struct WindData {
    float speed_mph   = 0.0f;    // wind speed (mph)
    float angle_deg   = 0.0f;    // relative angle (0–360, 0 = from bow/front)
    bool  available   = false;   // true when valid data received
    uint32_t last_update_ms = 0;
};

class WindSensor {
public:
    /// Initialise UART2 for Calypso.
    void begin();

    /// Call frequently to parse incoming NMEA data.
    void poll();

    const WindData& data() const { return data_; }

private:
    WindData data_;

    /// Parse a complete NMEA sentence.
    void parseNMEA(const char* sentence);

    /// Verify NMEA checksum.
    static bool verifyChecksum(const char* sentence);

    char buf_[83];         // NMEA max sentence length
    uint8_t buf_idx_ = 0;
};
