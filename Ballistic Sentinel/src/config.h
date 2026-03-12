#pragma once
/// @file config.h
/// @brief Hardware pin assignments and system constants for Ballistic Sentinel.
///
/// ESP32-WROOM-32 pin map:
///
///   I2C Bus (shared by OLED, BME280, QMC5883L)
///     SDA  → GPIO 21
///     SCL  → GPIO 22
///
///   5-Way Button (active-low with internal pull-up)
///     UP     → GPIO 32
///     DOWN   → GPIO 33
///     LEFT   → GPIO 25
///     RIGHT  → GPIO 26
///     CENTER → GPIO 27
///
///   Calypso Ultrasonic Anemometer (UART2, prepared — not yet connected)
///     RX ← GPIO 16
///     TX → GPIO 17
///
///   Status LED (on-board)
///     LED  → GPIO 2

#include <cstdint>

namespace cfg {

// ---------------------------------------------------------------------------
//  I2C
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_I2C_SDA       = 21;
constexpr uint8_t PIN_I2C_SCL       = 22;
constexpr uint32_t I2C_FREQ_HZ     = 400000;  // 400 kHz Fast-mode

// I2C addresses
constexpr uint8_t I2C_ADDR_OLED    = 0x3C;    // SH1106 default
constexpr uint8_t I2C_ADDR_BME280  = 0x76;    // SDO → GND
constexpr uint8_t I2C_ADDR_QMC5883 = 0x0D;    // fixed by chip
constexpr uint8_t I2C_ADDR_MPU6050 = 0x68;    // AD0 → GND

// ---------------------------------------------------------------------------
//  5-Way Navigation Button (active-low, use INPUT_PULLUP)
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_BTN_UP       = 32;
constexpr uint8_t PIN_BTN_DOWN     = 33;
constexpr uint8_t PIN_BTN_LEFT     = 25;
constexpr uint8_t PIN_BTN_RIGHT    = 26;
constexpr uint8_t PIN_BTN_CENTER   = 27;

// Debounce & long-press timing (ms)
constexpr uint16_t BTN_DEBOUNCE_MS      = 50;
constexpr uint16_t BTN_LONG_PRESS_MS    = 5000;  // deep sleep
constexpr uint16_t BTN_DOUBLE_PRESS_MS  = 300;   // double-press window
constexpr uint16_t BTN_REPEAT_DELAY_MS  = 400;   // auto-repeat start
constexpr uint16_t BTN_REPEAT_RATE_MS   = 150;   // auto-repeat interval

// ---------------------------------------------------------------------------
//  Calypso Wind Sensor (UART2) — prepared, not yet connected
// ---------------------------------------------------------------------------
constexpr uint8_t  PIN_WIND_RX     = 16;   // ESP32 RX ← Calypso TX
constexpr uint8_t  PIN_WIND_TX     = 17;   // ESP32 TX → Calypso RX
constexpr uint32_t WIND_BAUD       = 4800; // Calypso NMEA default

// ---------------------------------------------------------------------------
//  Status LED
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_LED          = 2;

// ---------------------------------------------------------------------------
//  WiFi Access Point
// ---------------------------------------------------------------------------
constexpr const char* WIFI_SSID     = "BallisticSentinel";
constexpr const char* WIFI_PASS     = "longrange";   // min 8 chars
constexpr uint16_t    WIFI_PORT     = 80;

// ---------------------------------------------------------------------------
//  Display
// ---------------------------------------------------------------------------
constexpr uint8_t DISPLAY_WIDTH    = 128;
constexpr uint8_t DISPLAY_HEIGHT   = 64;

// ---------------------------------------------------------------------------
//  Application Timing
// ---------------------------------------------------------------------------
constexpr uint16_t CALC_INTERVAL_MS = 5000;   // recalculate every 5 s
constexpr uint16_t DISPLAY_FPS      = 10;     // display refresh rate
constexpr uint16_t SENSOR_INTERVAL_MS = 200;  // sensor read interval (5 Hz)
constexpr uint8_t  CANT_CALIB_SAMPLES = 20;   // samples to average for cant calibration

// ---------------------------------------------------------------------------
//  NVS Storage Namespace
// ---------------------------------------------------------------------------
constexpr const char* NVS_NAMESPACE = "bs_config";

}  // namespace cfg
