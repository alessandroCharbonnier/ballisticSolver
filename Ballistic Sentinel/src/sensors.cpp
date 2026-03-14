#include "sensors.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <driver/adc.h>
#include <cmath>

// ── QMC5883P register definitions ─────────────────────────────────────────
namespace qmc5883p {
    constexpr uint8_t REG_CHIP_ID   = 0x00;  // should read 0x80
    constexpr uint8_t REG_DATA      = 0x01;  // 6 bytes: XL XH YL YH ZL ZH
    constexpr uint8_t REG_STATUS    = 0x09;  // bit 0 = DRDY
    constexpr uint8_t REG_CTRL1     = 0x0A;
    constexpr uint8_t REG_CTRL2     = 0x0B;
    constexpr uint8_t REG_SET_RESET = 0x29;

    constexpr uint8_t CHIP_ID_VAL   = 0x80;

    // CTRL1: OSR(7:6) | RNG(5:4) | ODR(3:2) | MODE(1:0)
    //   OSR=00 (512)  RNG=00 (2G)  ODR=11 (200Hz)  MODE=01 (continuous)
    constexpr uint8_t CTRL1_CONT_200HZ = 0x0D;
    //   OSR=00 (512)  RNG=00 (2G)  ODR=01 (10Hz)   MODE=01 (continuous)
    constexpr uint8_t CTRL1_CONT_10HZ  = 0x05;

    static void writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(cfg::I2C_ADDR_QMC5883P);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    static uint8_t readReg(uint8_t reg) {
        Wire.beginTransmission(cfg::I2C_ADDR_QMC5883P);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(cfg::I2C_ADDR_QMC5883P, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }

    static bool init() {
        // Verify chip ID
        uint8_t id = readReg(REG_CHIP_ID);
        Serial.printf("[sensors] QMC5883P chip ID: 0x%02X (expected 0x%02X)\n", id, CHIP_ID_VAL);
        if (id != CHIP_ID_VAL) return false;

        writeReg(REG_SET_RESET, 0x06);            // recommended SET/RESET
        writeReg(REG_CTRL1, CTRL1_CONT_200HZ);    // continuous, 200Hz, 2G, OSR512
        return true;
    }

    /// Read heading in degrees 0–360.  Returns -1 if not ready or read fails.
    static float readHeading() {
        // Check data-ready bit before reading
        if (!(readReg(REG_STATUS) & 0x01)) return -1.0f;

        Wire.beginTransmission(cfg::I2C_ADDR_QMC5883P);
        Wire.write(REG_DATA);
        Wire.endTransmission(false);
        if (Wire.requestFrom(cfg::I2C_ADDR_QMC5883P, (uint8_t)6) != 6) return -1.0f;

        int16_t x = (int16_t)(Wire.read() | (Wire.read() << 8));
        int16_t y = (int16_t)(Wire.read() | (Wire.read() << 8));
        // z bytes consumed but unused for 2D heading
        Wire.read(); Wire.read();

        float heading = atan2f((float)y, (float)x) * 180.0f / (float)M_PI;
        if (heading < 0.0f) heading += 360.0f;
        return heading;
    }
} // namespace qmc5883p

// ── Heading circular moving average (sin/cos to handle 0°/360° wrap) ─────
static constexpr int HDG_AVG_N = 20;       // samples in window
static float hdg_sin_buf[HDG_AVG_N];
static float hdg_cos_buf[HDG_AVG_N];
static float hdg_sin_sum = 0.0f;
static float hdg_cos_sum = 0.0f;
static int   hdg_idx     = 0;
static int   hdg_count   = 0;

static float headingSmooth(float deg) {
    float rad = deg * (float)M_PI / 180.0f;
    float s   = sinf(rad);
    float c   = cosf(rad);

    // Subtract the oldest sample being overwritten
    hdg_sin_sum -= hdg_sin_buf[hdg_idx];
    hdg_cos_sum -= hdg_cos_buf[hdg_idx];

    // Store & accumulate
    hdg_sin_buf[hdg_idx] = s;
    hdg_cos_buf[hdg_idx] = c;
    hdg_sin_sum += s;
    hdg_cos_sum += c;

    hdg_idx = (hdg_idx + 1) % HDG_AVG_N;
    if (hdg_count < HDG_AVG_N) hdg_count++;

    float avg = atan2f(hdg_sin_sum, hdg_cos_sum) * 180.0f / (float)M_PI;
    if (avg < 0.0f) avg += 360.0f;
    return avg;
}

// Module-level instances (hidden from header)
static Adafruit_BME280  s_bme;
static Adafruit_MPU6050 s_mpu;

void Sensors::begin() {
    // BME280
    data_.bme_ok = s_bme.begin(cfg::I2C_ADDR_BME280, &Wire);
    if (data_.bme_ok) {
        // Forced mode: sensor sleeps between on-demand reads (~1 µA idle)
        s_bme.setSampling(
            Adafruit_BME280::MODE_FORCED,
            Adafruit_BME280::SAMPLING_X2,   // temp
            Adafruit_BME280::SAMPLING_X16,  // pressure
            Adafruit_BME280::SAMPLING_X1,   // humidity
            Adafruit_BME280::FILTER_X16,
            Adafruit_BME280::STANDBY_MS_1000
        );
    }

    // QMC5883P compass
    Wire.beginTransmission(cfg::I2C_ADDR_QMC5883P);
    data_.compass_ok = (Wire.endTransmission() == 0) && qmc5883p::init();
    if (data_.compass_ok) {
        // Lower ODR to 10 Hz for power savings (was 200 Hz)
        qmc5883p::writeReg(qmc5883p::REG_CTRL1, qmc5883p::CTRL1_CONT_10HZ);
        Serial.println("[sensors] QMC5883P OK at 0x2C (10 Hz)");
    } else {
        Serial.println("[sensors] QMC5883P not found at 0x2C");
    }

    // MPU6050 (cant / roll)
    data_.mpu_ok = s_mpu.begin(cfg::I2C_ADDR_MPU6050, &Wire);
    if (data_.mpu_ok) {
        s_mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    // Battery ADC (GPIO36 / VP — ADC1_CH0)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    updateBattery();  // take an initial reading
}

void Sensors::update() {
    updateEnvironment();
    updateCant();
}

void Sensors::updateEnvironment() {
    // ── BME280 (forced mode — trigger measurement then read) ──────────────
    if (data_.bme_ok) {
        s_bme.takeForcedMeasurement();
        float temp_c      = s_bme.readTemperature();       // °C
        float press_hpa   = s_bme.readPressure() / 100.0f; // hPa
        float humidity    = s_bme.readHumidity();           // %

        data_.temperature_f = temp_c * 9.0f / 5.0f + 32.0f;
        data_.pressure_inhg = press_hpa * 0.02953f;        // hPa → inHg
        data_.humidity_pct  = humidity;
    }

    // ── QMC5883P compass ──────────────────────────────────────────────────
    if (data_.compass_ok) {
        float h = qmc5883p::readHeading();
        if (h >= 0.0f) data_.heading_deg = headingSmooth(h);
    }
}

void Sensors::updateCant() {
    // ── MPU6050 cant (roll) + motion detection ────────────────────────────
    if (data_.mpu_ok) {
        sensors_event_t a, g, temp;
        s_mpu.getEvent(&a, &g, &temp);
        float raw = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / (float)M_PI;

        // Motion detection: compare against previous accelerometer reading
        // Note: Adafruit MPU6050 getEvent() returns acceleration in m/s²
        float dx = a.acceleration.x - prev_ax_;
        float dy = a.acceleration.y - prev_ay_;
        float dz = a.acceleration.z - prev_az_;
        float delta = sqrtf(dx * dx + dy * dy + dz * dz);
        if (delta > cfg::MOTION_THRESHOLD_MPS2) {
            motion_detected_ = true;
        }
        prev_ax_ = a.acceleration.x;
        prev_ay_ = a.acceleration.y;
        prev_az_ = a.acceleration.z;

        // 1D Kalman filter
        if (!kf_init_) {
            kf_x_ = raw;
            kf_p_ = 1.0f;
            kf_init_ = true;
        } else {
            // Predict
            kf_p_ += KF_Q;
            // Update
            float k = kf_p_ / (kf_p_ + KF_R);
            kf_x_ += k * (raw - kf_x_);
            kf_p_ *= (1.0f - k);
        }
        data_.cant_deg = kf_x_;

        // Cant auto-calibration accumulator
        if (calib_remaining_ > 0) {
            calib_accum_ += data_.cant_deg;
            calib_remaining_--;
            if (calib_remaining_ == 0) {
                calib_result_ = calib_accum_ / (float)cfg::CANT_CALIB_SAMPLES;
                calib_done_ = true;
            }
        }
    }
}

bool Sensors::motionDetected() {
    bool m = motion_detected_;
    motion_detected_ = false;
    return m;
}

void Sensors::startCantCalibration() {
    calib_remaining_ = cfg::CANT_CALIB_SAMPLES;
    calib_accum_     = 0.0f;
    calib_done_      = false;
}

bool Sensors::cantCalibrationDone() {
    if (calib_done_) {
        calib_done_ = false;
        return true;
    }
    return false;
}

void Sensors::updateBattery() {
    // Multi-sample average to reduce ADC noise
    uint32_t sum = 0;
    for (uint8_t i = 0; i < cfg::BATT_AVG_SAMPLES; ++i) {
        sum += adc1_get_raw(ADC1_CHANNEL_0);
    }
    float raw_avg = (float)sum / (float)cfg::BATT_AVG_SAMPLES;

    // ESP32 ADC1 @ 11dB attenuation: ~0–3.3V mapped to 0–4095
    float adc_v = raw_avg * 3.3f / 4095.0f;
    float batt_v = adc_v * cfg::BATT_DIVIDER_RATIO;

    data_.battery_v = batt_v;

    // LiPo discharge curve (piecewise linear approximation)
    float pct;
    if (batt_v >= 4.15f)      pct = 100.0f;
    else if (batt_v >= 3.95f) pct = 80.0f + (batt_v - 3.95f) * (20.0f / 0.20f);
    else if (batt_v >= 3.80f) pct = 50.0f + (batt_v - 3.80f) * (30.0f / 0.15f);
    else if (batt_v >= 3.60f) pct = 20.0f + (batt_v - 3.60f) * (30.0f / 0.20f);
    else if (batt_v >= 3.30f) pct = 5.0f  + (batt_v - 3.30f) * (15.0f / 0.30f);
    else if (batt_v >= cfg::BATT_EMPTY_V) pct = (batt_v - cfg::BATT_EMPTY_V) * (5.0f / 0.30f);
    else                      pct = 0.0f;

    if (pct > 100.0f) pct = 100.0f;
    if (pct < 0.0f)   pct = 0.0f;
    data_.battery_pct = pct;
}
