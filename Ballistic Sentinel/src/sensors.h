#pragma once
/// @file sensors.h
/// @brief BME280 (temp/pressure/humidity) + QMC5883L (compass heading).

#include <cstdint>

struct SensorData {
    float temperature_f  = 59.0f;    // °F
    float pressure_inhg  = 29.92f;   // inHg
    float humidity_pct   = 0.0f;     // %
    float heading_deg    = 0.0f;     // compass heading 0–360
    float cant_deg       = 0.0f;     // rifle cant (roll from MPU6050)
    bool  bme_ok         = false;
    bool  compass_ok     = false;
    bool  mpu_ok         = false;
};

class Sensors {
public:
    void begin();

    /// Read all sensors.  Call at SENSOR_INTERVAL_MS.
    void update();

    /// Start cant auto-calibration (averages CANT_CALIB_SAMPLES readings).
    void startCantCalibration();

    /// True while calibration is running.
    bool cantCalibrating() const { return calib_remaining_ > 0; }

    /// True once after calibration finishes.  Calling clears the flag.
    bool cantCalibrationDone();

    /// The computed cant offset (valid after calibration completes).
    float cantCalibrationResult() const { return calib_result_; }

    const SensorData& data() const { return data_; }

private:
    SensorData data_;
    uint8_t calib_remaining_ = 0;
    float   calib_accum_     = 0.0f;
    float   calib_result_    = 0.0f;
    bool    calib_done_      = false;

    // 1D Kalman filter for cant smoothing
    bool  kf_init_  = false;
    float kf_x_     = 0.0f;   // state estimate (degrees)
    float kf_p_     = 1.0f;   // estimate uncertainty
    static constexpr float KF_Q = 0.5f;  // process noise (fast response for live level)
    static constexpr float KF_R = 0.8f;  // measurement noise (trust accel more)
};
