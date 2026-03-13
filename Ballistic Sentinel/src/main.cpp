/// @file main.cpp
/// @brief Ballistic Sentinel — entry point.
///
/// Ties together all sub-systems: display, input, sensors, wind, storage,
/// ballistic calculator (modes), and optional WiFi web-server.

#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

#include "config.h"
#include "display.h"
#include "input.h"
#include "sensors.h"
#include "storage.h"
#include "wind.h"
#include "webserver.h"
#include "modes.h"

// ═══════════════════════════════════════════════════════════════════════════
//  Globals
// ═══════════════════════════════════════════════════════════════════════════

static Display     g_display;
static Input       g_input;
Sensors            g_sensors;
static WindSensor  g_wind;
Storage            g_storage;
static WebServer_  g_web;
static ModeManager g_modes;

RifleConfig        g_rifle;
StageConfig        g_stages;

/// Set by the web-server REST handler when a new config is POSTed.
volatile bool g_config_changed = false;

// Timing
static uint32_t s_last_env_sensor  = 0;
static uint32_t s_last_cant_sensor = 0;
static uint32_t s_last_display     = 0;

// Activity tracking for auto-sleep
static uint32_t s_last_activity    = 0;

static bool     s_display_dirty    = true;
static bool     s_calc_needed      = true;

static constexpr uint32_t DISPLAY_INTERVAL_MS = 1000 / cfg::DISPLAY_FPS;

// ═══════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Reload config from NVS and push to the mode manager.
static void applyConfig() {
    g_modes.reconfigure(g_rifle, g_stages);
    Serial.println("[main] Config applied");
}

/// Push the latest mode state to the display.
static void updateDisplay() {
    g_display.setAppState(static_cast<uint8_t>(g_modes.appState()));
    g_display.setUnitDistance(g_modes.unitDistance());
    g_display.setUnitTemperature(g_modes.unitTemperature());
    g_display.setUnitPressure(g_modes.unitPressure());

    if (g_modes.appState() == AppState::MAIN_MENU) {
        g_display.setMenuCursor(g_modes.menuCursor());
        g_display.setWifiOn(g_modes.wifiOn());
    } else if (g_modes.appState() == AppState::SENSOR_VIEW) {
        const auto& sd = g_sensors.data();
        const auto& wd = g_wind.data();
        g_display.setSensors(sd.temperature_f, sd.pressure_inhg, sd.humidity_pct);
        g_display.setHeading(sd.heading_deg);
        g_display.setCant(sd.cant_deg);
        g_display.setCantCalibration(g_rifle.cant_offset,
                                      g_rifle.cant_sensitivity);
        g_display.setWind(wd.speed_mph, wd.angle_deg, wd.available);
    } else if (g_modes.appState() == AppState::DIGITAL_LEVEL) {
        const auto& sd = g_sensors.data();
        g_display.setCant(sd.cant_deg);
        g_display.setCantCalibration(g_rifle.cant_offset,
                                      g_rifle.cant_sensitivity);
    } else {
        bool staged = (g_modes.appState() == AppState::STAGE_SHOOTING);
        g_display.setMode(staged, g_modes.stageIndex(), g_modes.stageCount());

        if (staged) {
            g_display.setStageName(g_modes.stageName());
        }

        g_display.setDistance(g_modes.displayDistance());
        g_display.setCorrection(g_modes.result(), g_modes.corrUnit());
        g_display.setLiveDigitCursor(g_modes.digitCursor());

        const auto& sd = g_sensors.data();
        g_display.setSensors(sd.temperature_f, sd.pressure_inhg, sd.humidity_pct);
        g_display.setCant(sd.cant_deg);
        g_display.setCantCalibration(g_rifle.cant_offset,
                                      g_rifle.cant_sensitivity);
    }

    g_display.setWifiActive(g_web.isActive());
    g_display.update();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Setup
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Ballistic Sentinel ===");

    // I2C bus
    Wire.begin(cfg::PIN_I2C_SDA, cfg::PIN_I2C_SCL);
    Wire.setClock(cfg::I2C_FREQ_HZ);

    // Sub-system init
    g_display.begin();
    g_input.begin();
    g_sensors.begin();
    g_wind.begin();

    // Load saved config (or use defaults)
    if (!g_storage.load(g_rifle, g_stages)) {
        Serial.println("[main] No saved config — using defaults");
    } else {
        Serial.println("[main] Config loaded from NVS");
    }

    // Init ballistic engine with loaded config
    g_modes.begin(g_rifle, g_stages);

    // Initial sensor read + computation
    g_sensors.update();
    g_modes.compute(g_sensors.data(), g_wind.data());

    // LED off (we use it as WiFi indicator)
    pinMode(cfg::PIN_LED, OUTPUT);
    digitalWrite(cfg::PIN_LED, LOW);

    Serial.println("[main] Ready");

    s_last_activity = millis();

    // ── Wake-from-sleep validation ─────────────────────────────────────
    // After deep-sleep wakeup, require CENTER held for 5 s to fully boot.
    // If released early, go straight back to sleep.
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[main] Woke from deep sleep — hold CENTER 5s to confirm");
        const uint32_t wake_hold_ms = 5000;
        uint32_t hold_start = millis();
        bool confirmed = false;

        while (millis() - hold_start < wake_hold_ms) {
            if (digitalRead(cfg::PIN_BTN_CENTER) != LOW) {
                // Button released too early — go back to sleep
                Serial.println("[main] CENTER released — returning to sleep");
                g_display.clearScreen();
                g_input.configureWakeup();
                esp_deep_sleep_start();
            }
            uint8_t pct = (uint8_t)((millis() - hold_start) * 100 / wake_hold_ms);
            g_display.showWakeProgress(pct);
            delay(50);
        }
        Serial.println("[main] Wake confirmed");

        // Wait for CENTER to be released before entering loop,
        // otherwise poll() sees it as a new press event.
        while (digitalRead(cfg::PIN_BTN_CENTER) == LOW) {
            delay(10);
        }
        delay(cfg::BTN_DEBOUNCE_MS);  // let debounce settle
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Loop
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
    uint32_t now = millis();

    // ── Input ──────────────────────────────────────────────────────────────
    ButtonState btn = g_input.poll();

    if (btn.event != ButtonEvent::NONE) {
        static const char* const btn_names[] = {
            "NONE", "UP", "DOWN", "LEFT", "RIGHT", "CENTER"
        };
        static const char* const evt_names[] = {
            "NONE", "PRESS", "DOUBLE", "LONG", "REPEAT"
        };
        Serial.printf("[input] %s %s\n",
            btn_names[static_cast<uint8_t>(btn.id)],
            evt_names[static_cast<uint8_t>(btn.event)]);

        s_last_activity = now;
        s_display_dirty = true;

        // 5s hold CENTER → deep sleep
        if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::LONG_PRESS) {
            g_display.showShutdown();
            delay(1000);
            g_display.clearScreen();
            // Clear light-sleep wakeup sources before deep sleep
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            g_input.configureWakeup();  // ext0 only
            esp_deep_sleep_start();
        }

        g_modes.handleButton(btn);

        // WiFi toggled from menu
        if (g_modes.wifiToggled()) {
            if (g_modes.wifiOn()) {
                g_web.begin();
            } else {
                g_web.stop();
            }
            digitalWrite(cfg::PIN_LED, g_modes.wifiOn() ? HIGH : LOW);
        }
    }

    // ── Config hot-reload from web server ─────────────────────────────────
    if (g_config_changed) {
        g_config_changed = false;
        g_storage.load(g_rifle, g_stages);   // re-read what webserver saved
        applyConfig();
        s_calc_needed = true;
        s_display_dirty = true;
        s_last_activity = now;
    }

    // ── Wind sensor polling ───────────────────────────────────────────────
    g_wind.poll();

    // ── Captive portal DNS ────────────────────────────────────────────────
    g_web.processDNS();

    // ── Cant calibration completion ───────────────────────────────────────
    if (g_sensors.cantCalibrationDone()) {
        g_rifle.cant_offset = g_sensors.cantCalibrationResult();
        g_storage.save(g_rifle, g_stages);
        s_display_dirty = true;
        Serial.printf("[main] Cant calibrated: offset=%.2f\n",
                      (double)g_rifle.cant_offset);
    }

    // ── MPU6050 cant read (5 Hz — fast for live level indicator) ──────────
    if (now - s_last_cant_sensor >= cfg::CANT_SENSOR_INTERVAL_MS) {
        s_last_cant_sensor = now;
        g_sensors.updateCant();
        s_display_dirty = true;
    }

    // ── Environmental sensors (BME280 + compass, every 2 s) ──────────────
    if (now - s_last_env_sensor >= cfg::ENV_SENSOR_INTERVAL_MS) {
        s_last_env_sensor = now;
        g_sensors.updateEnvironment();
        s_display_dirty = true;
    }

    // ── Motion-based activity tracking ────────────────────────────────────
    if (g_sensors.motionDetected()) {
        s_last_activity = now;
        Serial.println("[power] Motion detected — activity timer reset");
    }

    // ── Auto-dim after inactivity ─────────────────────────────────────────
    bool should_dim = (now - s_last_activity > cfg::AUTO_DIM_TIMEOUT_MS);
    g_display.setDimmed(should_dim);

    // ── Auto deep-sleep on inactivity (no motion + no button) ─────────────
    if (!g_web.isActive()
        && (now - s_last_activity > cfg::AUTO_SLEEP_TIMEOUT_MS))
    {
        Serial.println("[main] No motion detected — entering deep sleep");
        g_display.showShutdown();
        delay(500);
        g_display.clearScreen();
        // Clear light-sleep wakeup sources before deep sleep
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        g_input.configureWakeup();  // ext0 only
        esp_deep_sleep_start();
    }

    // ── Ballistic calculation (on distance or config change only) ─────────
    if (g_modes.distanceChanged() || g_modes.configChanged() || s_calc_needed) {
        s_calc_needed = false;
        g_modes.compute(g_sensors.data(), g_wind.data());
        s_display_dirty = true;
    }

    // ── Display refresh ──────────────────────────────────────────────────
    if (s_display_dirty && (now - s_last_display >= DISPLAY_INTERVAL_MS)) {
        s_last_display = now;
        s_display_dirty = false;
        updateDisplay();
    }

    // ── Light sleep when idle (skip if WiFi AP is active) ────────────────
    if (!g_web.isActive()) {
        uint32_t next_cant    = s_last_cant_sensor + cfg::CANT_SENSOR_INTERVAL_MS;
        uint32_t next_env     = s_last_env_sensor  + cfg::ENV_SENSOR_INTERVAL_MS;
        uint32_t next_display = s_last_display     + DISPLAY_INTERVAL_MS;
        uint32_t next_wake    = next_cant;
        if (next_env < next_wake) next_wake = next_env;
        if (s_display_dirty && next_display < next_wake) next_wake = next_display;

        int32_t sleep_ms = static_cast<int32_t>(next_wake - now);
        if (sleep_ms > 10) {
            esp_sleep_enable_timer_wakeup(
                static_cast<uint64_t>(sleep_ms) * 1000ULL);
            // Button ISRs will also wake from light sleep
            esp_sleep_enable_gpio_wakeup();
            esp_light_sleep_start();
        }
    }
}