/// @file main.cpp
/// @brief Ballistic Sentinel — entry point.
///
/// Ties together all sub-systems: display, input, sensors, wind, storage,
/// ballistic calculator (modes), and optional WiFi web-server.

#include <Arduino.h>
#include <Wire.h>

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
static Sensors     g_sensors;
static WindSensor  g_wind;
Storage            g_storage;
static WebServer_  g_web;
static ModeManager g_modes;

RifleConfig        g_rifle;
StageConfig        g_stages;

/// Set by the web-server REST handler when a new config is POSTed.
volatile bool g_config_changed = false;

// Timing
static uint32_t s_last_calc    = 0;
static uint32_t s_last_sensor  = 0;
static uint32_t s_last_display = 0;

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
    bool staged = (g_modes.mode() == ShootingMode::STAGED);
    g_display.setMode(staged, g_modes.stageIndex(), g_modes.stageCount());

    if (staged) {
        g_display.setStageName(g_modes.stageName());
    }

    g_display.setDistance(g_modes.distance());
    g_display.setCorrection(g_modes.result(), g_modes.corrUnit());
    g_display.setLiveDigitCursor(g_modes.digitCursor());

    const auto& sd = g_sensors.data();
    g_display.setSensors(sd.temperature_f, sd.pressure_inhg, sd.humidity_pct);
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
}

// ═══════════════════════════════════════════════════════════════════════════
//  Loop
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
    uint32_t now = millis();

    // ── Input ──────────────────────────────────────────────────────────────
    ButtonState btn = g_input.poll();

    if (btn.event != ButtonEvent::NONE) {
        // Long-press CENTER → toggle WiFi AP
        if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::LONG_PRESS) {
            g_web.toggle();
            digitalWrite(cfg::PIN_LED, g_web.isActive() ? HIGH : LOW);
        } else {
            g_modes.handleButton(btn);
        }
    }

    // ── Config hot-reload from web server ─────────────────────────────────
    if (g_config_changed) {
        g_config_changed = false;
        g_storage.load(g_rifle, g_stages);   // re-read what webserver saved
        applyConfig();
    }

    // ── Wind sensor polling ───────────────────────────────────────────────
    g_wind.poll();

    // ── Sensor read (1 Hz) ────────────────────────────────────────────────
    if (now - s_last_sensor >= cfg::SENSOR_INTERVAL_MS) {
        s_last_sensor = now;
        g_sensors.update();
    }

    // ── Ballistic calculation (1 Hz) ─────────────────────────────────────
    if (now - s_last_calc >= cfg::CALC_INTERVAL_MS) {
        s_last_calc = now;
        g_modes.compute(g_sensors.data(), g_wind.data());
    }

    // ── Display refresh (10 fps) ─────────────────────────────────────────
    if (now - s_last_display >= DISPLAY_INTERVAL_MS) {
        s_last_display = now;
        updateDisplay();
    }
}