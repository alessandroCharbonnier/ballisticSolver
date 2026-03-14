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
///   Battery ADC (voltage divider: 100kΩ + 100kΩ)
///     ADC  ← GPIO 36 (VP / ADC1_CH0)
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
constexpr uint8_t I2C_ADDR_QMC5883P = 0x2C;   // QMC5883P default
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
constexpr uint32_t WAKE_HOLD_MS         = 3000;  // wake hold time
constexpr uint16_t BTN_LONG_PRESS_MS    = 3000;  // deep sleep
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
constexpr uint16_t DISPLAY_FPS        = 4;      // display refresh rate
constexpr uint16_t ENV_SENSOR_INTERVAL_MS  = 2000;  // BME280 + compass (0.5 Hz)
constexpr uint16_t CANT_SENSOR_INTERVAL_MS = 200;   // MPU6050 cant only (5 Hz)
constexpr uint8_t  CANT_CALIB_SAMPLES = 20;   // samples to average for cant calibration

// ---------------------------------------------------------------------------
//  Auto-Dim & Auto-Sleep (motion-based)
// ---------------------------------------------------------------------------
constexpr uint32_t AUTO_DIM_TIMEOUT_MS     = 120000;  // 2min → dim display
constexpr uint32_t AUTO_SLEEP_TIMEOUT_MS   = 600000;  // 10min without motion → deep sleep
constexpr uint8_t  DISPLAY_CONTRAST_FULL   = 255;    // normal brightness
constexpr uint8_t  DISPLAY_CONTRAST_DIM    = 35;     // ~20% of full (~80% reduction)
constexpr float    MOTION_THRESHOLD_MPS2   = 2.0f;   // accel delta in m/s² (~0.43g)

// ---------------------------------------------------------------------------
//  Battery ADC (voltage divider: 100kΩ + 100kΩ on GPIO36/VP)
// ---------------------------------------------------------------------------
constexpr uint8_t  PIN_BATTERY_ADC         = 36;     // GPIO36 (VP) — ADC1_CH0
constexpr float    BATT_DIVIDER_RATIO      = 2.0f;   // R1=R2=100kΩ → ratio 2:1
constexpr float    BATT_FULL_V             = 4.2f;   // fully charged LiPo
constexpr float    BATT_EMPTY_V            = 3.0f;   // cutoff voltage
constexpr uint16_t BATT_READ_INTERVAL_MS   = 10000;  // sample every 10 s
constexpr uint8_t  BATT_AVG_SAMPLES        = 8;      // multi-sample average

// ---------------------------------------------------------------------------
//  NVS Storage Namespace
// ---------------------------------------------------------------------------
constexpr const char* NVS_NAMESPACE = "bs_config";

}  // namespace cfg
