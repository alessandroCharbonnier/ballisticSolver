#pragma once
/// @file sensors.h
/// @brief BME280 (temp/pressure/humidity) + QMC5883P (compass heading).

#include <cstdint>

struct SensorData {
    float temperature_f  = 59.0f;    // °F
    float pressure_inhg  = 29.92f;   // inHg
    float humidity_pct   = 0.0f;     // %
    float heading_deg    = 0.0f;     // compass heading 0–360
    float cant_deg       = 0.0f;     // rifle cant (roll from MPU6050)
    float battery_pct    = -1.0f;    // battery 0–100 (−1 = not available)
    float battery_v      = 0.0f;     // raw battery voltage
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

    // ── Compass (magnetometer) calibration ───────────────────────────────

    /// Start compass calibration: user must rotate sensor 360° on a flat surface.
    void startCompassCalibration();

    /// Finish compass calibration: compute offsets and scales from collected data.
    void finishCompassCalibration();

    /// True while compass calibration is in progress.
    bool compassCalibrating() const { return mag_cal_active_; }

    /// True once after compass calibration finishes.  Clears on read.
    bool compassCalibrationDone();

    /// Set pre-loaded calibration offsets (from NVS).
    void setCompassCalibration(float off_x, float off_y,
                               float scl_x, float scl_y);

    /// Get calibration results.
    float magOffsetX() const { return mag_off_x_; }
    float magOffsetY() const { return mag_off_y_; }
    float magScaleX()  const { return mag_scl_x_; }
    float magScaleY()  const { return mag_scl_y_; }

    const SensorData& data() const { return data_; }

    /// Read only the MPU6050 cant (fast, for high-rate polling).
    void updateCant();

    /// Read only the QMC5883P compass heading (fast I2C, 5–10 Hz).
    void updateCompass();

    /// Read BME280 (slow forced-mode environmental sensors).
    void updateEnvironment();

    /// Read battery voltage via ADC.
    void updateBattery();

    /// Returns true if significant motion was detected since last call.
    bool motionDetected();

private:
    void compassCalibrationTick_(float rx, float ry);

    float prev_ax_ = 0.0f;
    float prev_ay_ = 0.0f;
    float prev_az_ = 0.0f;
    bool  motion_detected_ = false;
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

    // Compass calibration state
    bool  mag_cal_active_ = false;
    bool  mag_cal_done_   = false;
    float mag_min_x_ =  1e9f;
    float mag_max_x_ = -1e9f;
    float mag_min_y_ =  1e9f;
    float mag_max_y_ = -1e9f;
    uint16_t mag_cal_samples_ = 0;

    // Compass calibration results (applied during heading calc)
    float mag_off_x_ = 0.0f;   // hard iron offset X
    float mag_off_y_ = 0.0f;   // hard iron offset Y
    float mag_scl_x_ = 1.0f;   // soft iron scale X
    float mag_scl_y_ = 1.0f;   // soft iron scale Y
};
