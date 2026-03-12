#pragma once
/// @file sensors.h
/// @brief BME280 (temp/pressure/humidity) + QMC5883L (compass heading).

#include <cstdint>

struct SensorData {
    float temperature_f  = 59.0f;    // °F
    float pressure_inhg  = 29.92f;   // inHg
    float humidity_pct   = 0.0f;     // %
    float heading_deg    = 0.0f;     // compass heading 0–360
    bool  bme_ok         = false;
    bool  compass_ok     = false;
};

class Sensors {
public:
    void begin();

    /// Read all sensors.  Call at SENSOR_INTERVAL_MS.
    void update();

    const SensorData& data() const { return data_; }

private:
    SensorData data_;
};
