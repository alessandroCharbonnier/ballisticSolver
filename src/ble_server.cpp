/// @file ble_server.cpp
/// @brief NimBLE peripheral (phone config + telemetry) + weather-dongle scan.
///
/// Two independent BLE jobs share one NimBLE stack:
///   1. Weather dongle: a periodic 2 s scan window (every 10 s) reads the
///      dongle's manufacturer-data beacon and feeds `g_sensors`. If the
///      dongle goes silent for `WEATHER_STALE_MS`, we fall back to the manual
///      weather values stored in `g_rifle`.
///   2. Phone app: an on-demand advertising peripheral. The app-layer PIN
///      (shown on the OLED) gates config/command writes; config is exchanged
///      as a single JSON document over the config characteristic.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <ballistic.h>
#include <cstdio>
#include <cstring>

#include "ble_server.h"
#include "config.h"
#include "storage.h"

// ── Globals owned by main.cpp ───────────────────────────────────────────────
extern RifleConfig  g_rifle;
extern StageConfig  g_stages;
extern Storage      g_storage;
extern Sensors      g_sensors;
extern volatile bool g_config_changed;

// ── GATT layout (shared with the phone app) ─────────────────────────────────
static constexpr const char* SERVICE_UUID    = "19B10000-E8F2-537E-4F6C-D104768A1214";
static constexpr const char* CHAR_CONFIG     = "19B10001-E8F2-537E-4F6C-D104768A1214";
static constexpr const char* CHAR_TELEMETRY  = "19B10002-E8F2-537E-4F6C-D104768A1214";
static constexpr const char* CHAR_COMMAND    = "19B10003-E8F2-537E-4F6C-D104768A1214";
static constexpr const char* CHAR_STATUS     = "19B10004-E8F2-537E-4F6C-D104768A1214";

// ── Weather dongle beacon payload (little-endian, mirrors the dongle) ──────
#pragma pack(push, 1)
struct WeatherPayload {
    float    temp_c;
    float    pressure_hpa;
    float    humidity_pct;
    uint32_t seq;
    uint32_t millis_ms;
};
#pragma pack(pop)
static_assert(sizeof(WeatherPayload) == 20, "WeatherPayload packing");

// 1 hPa = 29.92 inHg / 1013.25 hPa
static constexpr float kHpaToInhg = 29.92f / 1013.25f;

// ── Module state (file-scope; the class is a thin facade) ──────────────────
static NimBLECharacteristic* s_config_ch    = nullptr;
static NimBLECharacteristic* s_telemetry_ch = nullptr;
static NimBLECharacteristic* s_command_ch   = nullptr;
static NimBLECharacteristic* s_status_ch    = nullptr;

static bool     s_advertising    = false;
static bool     s_authenticated  = false;
static bool     s_weather_live   = false;
static bool     s_telemetry_subscribed = false;
static uint32_t s_weather_last_ms = 0;
static uint32_t s_last_scan_ms    = 0;

// ═══════════════════════════════════════════════════════════════════════════
//  JSON config (de)serialisation
// ═══════════════════════════════════════════════════════════════════════════

static String configToJson(const RifleConfig& r, const StageConfig& s) {
    JsonDocument doc;

    JsonObject cfg = doc["config"].to<JsonObject>();
    cfg["bc"]         = r.bc;
    cfg["drag_table"] = ballistic::dragTableName(
        static_cast<ballistic::DragTableId>(r.drag_table));
    cfg["weight"]     = r.weight_gr;
    cfg["caliber"]    = r.diameter_in;
    cfg["length"]     = r.length_in;
    cfg["mv"]         = r.muzzle_vel_fps;
    cfg["sh"]         = r.sight_height_in;
    cfg["twist"]      = r.twist_in;
    cfg["zero"]       = r.zero_range_yd;
    cfg["lat"]        = r.latitude_deg;
    cfg["corr_unit"]  = ballistic::correctionUnitLabel(
        static_cast<ballistic::CorrectionUnit>(r.correction_unit));
    cfg["click_size"] = r.click_size_moa;
    cfg["click_unit"] = r.click_unit;
    cfg["cant_offset"] = r.cant_offset;
    cfg["cant_sens"]   = r.cant_sensitivity;
    cfg["multi_bc"]    = r.multi_bc;
    cfg["use_ps"]      = r.use_powder_sens;
    cfg["ble_pin"]     = r.ble_pin;

    JsonObject units = cfg["units"].to<JsonObject>();
    units["distance"]    = r.unit_distance;
    units["velocity"]    = r.unit_velocity;
    units["weight"]      = r.unit_weight;
    units["length"]      = r.unit_length;
    units["temperature"] = r.unit_temperature;
    units["pressure"]    = r.unit_pressure;

    JsonObject man = cfg["manual"].to<JsonObject>();
    man["temp_f"]        = r.manual_temp_f;
    man["pressure_inhg"] = r.manual_pressure_inhg;
    man["humidity"]      = r.manual_humidity_pct;

    if (r.dongle_mac_set) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 r.dongle_mac[0], r.dongle_mac[1], r.dongle_mac[2],
                 r.dongle_mac[3], r.dongle_mac[4], r.dongle_mac[5]);
        cfg["dongle_mac"] = mac;
    } else {
        cfg["dongle_mac"] = "";
    }

    JsonArray stages = doc["stages"].to<JsonArray>();
    for (uint8_t i = 0; i < s.count; ++i) {
        JsonObject st = stages.add<JsonObject>();
        st["name"]     = s.targets[i].name;
        st["distance"] = s.targets[i].distance_yd;
    }

    String out;
    serializeJson(doc, out);
    return out;
}

/// Parse a config JSON document into `r`/`s` in place (missing fields keep
/// their current values). Returns false on malformed JSON.
static bool configFromJson(const uint8_t* data, size_t len,
                           RifleConfig& r, StageConfig& s) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len)) return false;

    JsonObject cfg = doc["config"];
    if (!cfg.isNull()) {
        r.bc = cfg["bc"] | r.bc;

        const char* dt = cfg["drag_table"];
        if (dt) {
            r.drag_table = static_cast<uint8_t>(
                ballistic::dragTableFromName(dt));
        }

        r.weight_gr       = cfg["weight"]    | r.weight_gr;
        r.diameter_in     = cfg["caliber"]   | r.diameter_in;
        r.length_in       = cfg["length"]    | r.length_in;
        r.muzzle_vel_fps  = cfg["mv"]        | r.muzzle_vel_fps;
        r.sight_height_in = cfg["sh"]        | r.sight_height_in;
        r.twist_in        = cfg["twist"]     | r.twist_in;
        r.zero_range_yd   = cfg["zero"]      | r.zero_range_yd;
        r.latitude_deg    = cfg["lat"]       | r.latitude_deg;

        const char* cu = cfg["corr_unit"];
        if (cu) {
            r.correction_unit = static_cast<uint8_t>(
                ballistic::correctionUnitFromStr(cu));
        }

        r.click_size_moa    = cfg["click_size"] | r.click_size_moa;
        r.click_unit        = cfg["click_unit"] | r.click_unit;
        r.cant_offset       = cfg["cant_offset"] | r.cant_offset;
        r.cant_sensitivity  = cfg["cant_sens"]   | r.cant_sensitivity;
        r.multi_bc          = cfg["multi_bc"]    | r.multi_bc;
        r.use_powder_sens   = cfg["use_ps"]      | r.use_powder_sens;
        r.ble_pin           = cfg["ble_pin"]     | r.ble_pin;

        JsonObject units = cfg["units"];
        if (!units.isNull()) {
            r.unit_distance    = units["distance"]    | r.unit_distance;
            r.unit_velocity    = units["velocity"]    | r.unit_velocity;
            r.unit_weight      = units["weight"]      | r.unit_weight;
            r.unit_length      = units["length"]      | r.unit_length;
            r.unit_temperature = units["temperature"] | r.unit_temperature;
            r.unit_pressure    = units["pressure"]    | r.unit_pressure;
        }

        JsonObject man = cfg["manual"];
        if (!man.isNull()) {
            r.manual_temp_f        = man["temp_f"]        | r.manual_temp_f;
            r.manual_pressure_inhg = man["pressure_inhg"] | r.manual_pressure_inhg;
            r.manual_humidity_pct  = man["humidity"]      | r.manual_humidity_pct;
        }

        const char* mac = cfg["dongle_mac"];
        if (mac && mac[0] != '\0') {
            unsigned m[6] = {0};
            if (sscanf(mac, "%2x:%2x:%2x:%2x:%2x:%2x",
                       &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6) {
                for (int i = 0; i < 6; ++i) r.dongle_mac[i] = (uint8_t)m[i];
                r.dongle_mac_set = true;
            }
        } else {
            r.dongle_mac_set = false;
            memset(r.dongle_mac, 0, sizeof(r.dongle_mac));
        }
    }

    JsonArray stages = doc["stages"];
    if (!stages.isNull()) {
        s.count = 0;
        for (JsonVariant v : stages) {
            if (s.count >= MAX_STAGES) break;
            JsonObject st = v.as<JsonObject>();
            const char* name = st["name"] | "";
            strncpy(s.targets[s.count].name, name, 16);
            s.targets[s.count].name[16] = '\0';
            s.targets[s.count].distance_yd =
                static_cast<uint16_t>(st["distance"] | 0);
            ++s.count;
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Weather dongle scan
// ═══════════════════════════════════════════════════════════════════════════

/// True if the advertised device is our weather dongle.
static bool isWeatherDongle(const NimBLEAdvertisedDevice& dev) {
    if (!dev.haveManufacturerData()) return false;

    const std::string mfr = dev.getManufacturerData();
    if (mfr.size() < 2 + sizeof(WeatherPayload)) return false;

    const uint16_t company =
        static_cast<uint8_t>(mfr[0]) |
        (static_cast<uint16_t>(static_cast<uint8_t>(mfr[1])) << 8);
    if (company != cfg::WEATHER_COMPANY_ID) return false;

    // If the user bound a specific dongle, match its MAC exactly;
    // otherwise accept any dongle advertising our name + company id.
    if (g_rifle.dongle_mac_set) {
        const NimBLEAddress bound(g_rifle.dongle_mac, BLE_ADDR_RANDOM);
        return dev.getAddress() == bound;
    }
    return dev.getName() == cfg::WEATHER_DONGLE_NAME;
}

/// Feed a freshly-seen dongle beacon into the sensor model.
static void acceptWeather(const NimBLEAdvertisedDevice& dev) {
    const std::string mfr = dev.getManufacturerData();
    const WeatherPayload* wp =
        reinterpret_cast<const WeatherPayload*>(mfr.data() + 2);

    const float temp_f = wp->temp_c * 9.0f / 5.0f + 32.0f;
    const float press_inhg = wp->pressure_hpa * kHpaToInhg;

    g_sensors.setWeather(temp_f, press_inhg, wp->humidity_pct, true);
    s_weather_live = true;
    s_weather_last_ms = millis();
}

class WeatherScanCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice* dev) override {
        if (dev && isWeatherDongle(*dev)) acceptWeather(*dev);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  GATT callbacks
// ═══════════════════════════════════════════════════════════════════════════

static void notifyStatus(const char* msg) {
    if (!s_status_ch) return;
    s_status_ch->setValue(msg);
    s_status_ch->notify();
}

class ConfigCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onRead(NimBLECharacteristic* ch, NimBLEConnInfo& connInfo) override {
        (void)connInfo;
        const String json = configToJson(g_rifle, g_stages);
        ch->setValue(json.c_str());
    }

    void onWrite(NimBLECharacteristic* ch, NimBLEConnInfo& connInfo) override {
        (void)connInfo;
        if (!s_authenticated) { notifyStatus("auth required"); return; }

        const NimBLEAttValue val = ch->getValue();
        RifleConfig r = g_rifle;
        StageConfig s = g_stages;
        if (!configFromJson(val.data(), val.size(), r, s)) {
            notifyStatus("bad json");
            return;
        }

        // Persist, then let the main loop reload + re-apply (single-writer).
        g_storage.save(r, s);
        g_config_changed = true;
        notifyStatus("saved");
    }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* ch, NimBLEConnInfo& connInfo) override {
        (void)connInfo;
        const NimBLEAttValue val = ch->getValue();

        JsonDocument doc;
        if (deserializeJson(doc, val.data(), val.size())) {
            notifyStatus("bad json");
            return;
        }

        const char* cmd = doc["cmd"] | "";
        if (strcmp(cmd, "auth") == 0) {
            const uint32_t pin = doc["pin"] | 0xFFFFFFFFu;
            if (pin == g_rifle.ble_pin) {
                s_authenticated = true;
                notifyStatus("authed");
            } else {
                s_authenticated = false;
                notifyStatus("bad pin");
            }
            return;
        }

        if (!s_authenticated) { notifyStatus("auth required"); return; }

        if (strcmp(cmd, "cant_cal") == 0) {
            g_sensors.startCantCalibration();
            notifyStatus("cant_cal started");
        } else {
            notifyStatus("unknown cmd");
        }
    }
};

class TelemetryCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onSubscribe(NimBLECharacteristic* ch, NimBLEConnInfo& connInfo,
                     uint16_t subValue) override {
        (void)ch; (void)connInfo;
        s_telemetry_subscribed = (subValue != 0);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════

void BleServer::begin() {
    NimBLEDevice::init(cfg::BLE_DEVICE_NAME);

    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* svc = server->createService(SERVICE_UUID);

    s_config_ch = svc->createCharacteristic(
        CHAR_CONFIG, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    s_config_ch->setCallbacks(new ConfigCallbacks());

    s_telemetry_ch = svc->createCharacteristic(
        CHAR_TELEMETRY, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    s_telemetry_ch->setCallbacks(new TelemetryCallbacks());

    s_command_ch = svc->createCharacteristic(
        CHAR_COMMAND, NIMBLE_PROPERTY::WRITE);
    s_command_ch->setCallbacks(new CommandCallbacks());

    s_status_ch = svc->createCharacteristic(
        CHAR_STATUS, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    // Advertising is configured up-front but only started on demand.
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setName(cfg::BLE_DEVICE_NAME);
    adv->addServiceUUID(SERVICE_UUID);
    adv->enableScanResponse(true);

    // Weather scan: active scan so the dongle's name (carried in the scan
    // response) is received alongside its manufacturer data.
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    scan->setScanCallbacks(new WeatherScanCallbacks(), false);

    // Start from manual weather until a dongle is heard.
    applyManualWeather_();
    s_last_scan_ms = millis();

    Serial.println("[ble] NimBLE ready (advertising off until toggled)");
}

void BleServer::update() {
    const uint32_t now = millis();

    // Staleness fallback: dongle silent for too long → manual weather.
    if (s_weather_live &&
        (now - s_weather_last_ms > cfg::WEATHER_STALE_MS)) {
        applyManualWeather_();
    }

    // Periodic scan window.
    if (now - s_last_scan_ms >= cfg::WEATHER_SCAN_INTERVAL_MS) {
        s_last_scan_ms = now;
        NimBLEScan* scan = NimBLEDevice::getScan();
        if (scan) scan->start(cfg::WEATHER_SCAN_WINDOW_MS, false);
    }
}

void BleServer::startAdvertising() {
    if (s_advertising) return;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (!adv) return;
    s_advertising = adv->start();
    if (!s_advertising) Serial.println("[ble] advertising start failed");
}

void BleServer::stopAdvertising() {
    if (!s_advertising) return;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (adv) adv->stop();
    s_advertising = false;
    s_authenticated = false;
}

bool BleServer::isAdvertising() const { return s_advertising; }

void BleServer::publishTelemetry(const SensorData& sd, float distance,
                                 float v_corr, float h_corr, bool staged) {
    if (!s_telemetry_ch || !s_telemetry_subscribed) return;

    JsonDocument doc;
    doc["t_f"]      = sd.temperature_f;
    doc["p_inhg"]   = sd.pressure_inhg;
    doc["rh_pct"]   = sd.humidity_pct;
    doc["head_deg"] = sd.heading_deg;
    doc["cant_deg"] = sd.cant_deg;
    doc["batt_pct"] = sd.battery_pct;
    doc["batt_v"]   = sd.battery_v;
    doc["wx_live"]  = s_weather_live;
    doc["distance"] = distance;
    doc["v_corr"]   = v_corr;
    doc["h_corr"]   = h_corr;
    doc["staged"]   = staged;

    String out;
    serializeJson(doc, out);
    s_telemetry_ch->setValue(out.c_str());
    s_telemetry_ch->notify();
}

uint32_t BleServer::pin() const { return g_rifle.ble_pin; }

bool BleServer::weatherLive() const { return s_weather_live; }

void BleServer::applyManualWeather_() {
    s_weather_live = false;
    g_sensors.setWeather(g_rifle.manual_temp_f,
                         g_rifle.manual_pressure_inhg,
                         g_rifle.manual_humidity_pct,
                         false);
}
