#include "sensors.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <QMC5883LCompass.h>

// Module-level instances (hidden from header)
static Adafruit_BME280  s_bme;
static QMC5883LCompass  s_compass;

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
}
