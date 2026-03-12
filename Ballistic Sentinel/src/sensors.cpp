#include "sensors.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <QMC5883LCompass.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <cmath>

// Module-level instances (hidden from header)
static Adafruit_BME280  s_bme;
static QMC5883LCompass  s_compass;
static Adafruit_MPU6050 s_mpu;

void Sensors::begin() {
    // BME280
    data_.bme_ok = s_bme.begin(cfg::I2C_ADDR_BME280, &Wire);
    if (data_.bme_ok) {
        // Weather-station mode: low noise, slow sampling
        s_bme.setSampling(
            Adafruit_BME280::MODE_NORMAL,
            Adafruit_BME280::SAMPLING_X2,   // temp
            Adafruit_BME280::SAMPLING_X16,  // pressure
            Adafruit_BME280::SAMPLING_X1,   // humidity
            Adafruit_BME280::FILTER_X16,
            Adafruit_BME280::STANDBY_MS_1000
        );
    }

    // QMC5883L compass
    s_compass.init();
    s_compass.setSmoothing(10, true);
    data_.compass_ok = true;  // library doesn't provide a status flag

    // MPU6050 (cant / roll)
    data_.mpu_ok = s_mpu.begin(cfg::I2C_ADDR_MPU6050, &Wire);
    if (data_.mpu_ok) {
        s_mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
}

void Sensors::update() {
    // ── BME280 ────────────────────────────────────────────────────────────
    if (data_.bme_ok) {
        float temp_c      = s_bme.readTemperature();       // °C
        float press_hpa   = s_bme.readPressure() / 100.0f; // hPa
        float humidity    = s_bme.readHumidity();           // %

        data_.temperature_f = temp_c * 9.0f / 5.0f + 32.0f;
        data_.pressure_inhg = press_hpa * 0.02953f;        // hPa → inHg
        data_.humidity_pct  = humidity;
    }

    // ── QMC5883L compass ──────────────────────────────────────────────────
    if (data_.compass_ok) {
        s_compass.read();
        data_.heading_deg = static_cast<float>(s_compass.getAzimuth());
    }

    // ── MPU6050 cant (roll) ───────────────────────────────────────────────
    if (data_.mpu_ok) {
        sensors_event_t a, g, temp;
        s_mpu.getEvent(&a, &g, &temp);
        data_.cant_deg = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / (float)M_PI;
    }
}
