#include "webserver.h"
#include "web_ui.h"
#include "config.h"
#include "storage.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Forward-declare the global config objects (owned by main.cpp)
extern RifleConfig  g_rifle;
extern StageConfig  g_stages;
extern Storage      g_storage;
extern volatile bool g_config_changed;

static AsyncWebServer* s_server = nullptr;

// ═══════════════════════════════════════════════════════════════════════════
//  Request handlers
// ═══════════════════════════════════════════════════════════════════════════

static void handleIndex(AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", INDEX_HTML);
}

/// GET /api/config — return current config + stages as JSON.
static void handleGetConfig(AsyncWebServerRequest* req) {
    JsonDocument doc;

    // Config object
    JsonObject cfg = doc["config"].to<JsonObject>();
    cfg["bc"]         = g_rifle.bc;
    cfg["drag_table"] = ballistic::dragTableName(
        static_cast<ballistic::DragTableId>(g_rifle.drag_table));
    cfg["weight"]     = g_rifle.weight_gr;
    cfg["caliber"]    = g_rifle.diameter_in;
    cfg["length"]     = g_rifle.length_in;
    cfg["mv"]         = g_rifle.muzzle_vel_fps;
    cfg["sh"]         = g_rifle.sight_height_in;
    cfg["twist"]      = g_rifle.twist_in;
    cfg["zero"]       = g_rifle.zero_range_yd;
    cfg["lat"]        = g_rifle.latitude_deg;
    cfg["corr_unit"]  = ballistic::correctionUnitLabel(
        static_cast<ballistic::CorrectionUnit>(g_rifle.correction_unit));
    cfg["click_size"] = g_rifle.click_size_moa;
    cfg["multi_bc"]   = g_rifle.multi_bc;
    cfg["use_ps"]     = g_rifle.use_powder_sens;
    cfg["unit_system"] = g_rifle.unit_system;

    if (g_rifle.multi_bc && g_rifle.num_bc_points > 0) {
        JsonArray bca = cfg["bc_points"].to<JsonArray>();
        for (uint8_t i = 0; i < g_rifle.num_bc_points && i < 5; ++i) {
            JsonObject p = bca.add<JsonObject>();
            p["bc"]  = g_rifle.bc_points[i];
            p["vel"] = g_rifle.bc_velocities[i];
        }
    }

    if (g_rifle.use_powder_sens) {
        JsonObject ps = cfg["ps"].to<JsonObject>();
        ps["v1"] = g_rifle.muzzle_vel_fps;
        ps["t1"] = g_rifle.powder_temp_f;
    }

    // Stages
    JsonArray stages = doc["stages"].to<JsonArray>();
    for (uint8_t i = 0; i < g_stages.count; ++i) {
        JsonObject s = stages.add<JsonObject>();
        s["name"]     = g_stages.targets[i].name;
        s["distance"] = g_stages.targets[i].distance_yd;
    }

    String output;
    serializeJson(doc, output);
    req->send(200, "application/json", output);
}

/// POST /api/save — receive config + stages JSON.
static void handleSave(AsyncWebServerRequest* req, uint8_t* data,
                        size_t len, size_t /*index*/, size_t /*total*/)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        req->send(400, "text/plain", "Invalid JSON");
        return;
    }

    // Parse config
    JsonObject cfg = doc["config"];
    if (!cfg.isNull()) {
        g_rifle.bc             = cfg["bc"]       | 0.315f;
        const char* dt_str     = cfg["drag_table"] | "G7";
        g_rifle.drag_table     = static_cast<uint8_t>(
            ballistic::dragTableFromName(dt_str));
        g_rifle.weight_gr      = cfg["weight"]   | 140.0f;
        g_rifle.diameter_in    = cfg["caliber"]   | 0.264f;
        g_rifle.length_in      = cfg["length"]    | 1.35f;
        g_rifle.muzzle_vel_fps = cfg["mv"]        | 2710.0f;
        g_rifle.sight_height_in= cfg["sh"]        | 1.5f;
        g_rifle.twist_in       = cfg["twist"]     | 8.0f;
        g_rifle.zero_range_yd  = cfg["zero"]      | 100.0f;
        g_rifle.latitude_deg   = cfg["lat"]       | 0.0f;
        const char* cu_str     = cfg["corr_unit"] | "MOA";
        g_rifle.correction_unit= static_cast<uint8_t>(
            ballistic::correctionUnitFromStr(cu_str));
        g_rifle.click_size_moa = cfg["click_size"] | 0.25f;
        g_rifle.multi_bc       = cfg["multi_bc"]   | false;
        g_rifle.use_powder_sens= cfg["use_ps"]     | false;
        g_rifle.unit_system    = cfg["unit_system"] | 0;

        // Multi-BC
        g_rifle.num_bc_points = 0;
        if (g_rifle.multi_bc && cfg["bc_points"].is<JsonArray>()) {
            JsonArray bca = cfg["bc_points"];
            for (size_t i = 0; i < bca.size() && i < 5; ++i) {
                g_rifle.bc_points[i]     = bca[i]["bc"]  | 0.0f;
                g_rifle.bc_velocities[i] = bca[i]["vel"] | 0.0f;
                if (g_rifle.bc_points[i] > 0.0f) g_rifle.num_bc_points++;
            }
        }

        // Powder sensitivity (calculate modifier from two readings)
        if (g_rifle.use_powder_sens && !cfg["ps"].isNull()) {
            float v1 = cfg["ps"]["v1"] | 0.0f;
            float t1 = cfg["ps"]["t1"] | 0.0f;
            float v2 = cfg["ps"]["v2"] | 0.0f;
            float t2 = cfg["ps"]["t2"] | 0.0f;
            float dt_val = fabsf(t1 - t2);
            if (dt_val > 0.0f && v1 > 0.0f && v2 > 0.0f) {
                float dv = fabsf(v1 - v2);
                float v_min = fminf(v1, v2);
                g_rifle.powder_modifier = (dv / dt_val) * (15.0f / v_min);
                g_rifle.powder_temp_f = t1;
            }
        }
    }

    // Parse stages
    if (doc["stages"].is<JsonArray>()) {
        JsonArray stg = doc["stages"];
        g_stages.count = 0;
        for (size_t i = 0; i < stg.size() && i < MAX_STAGES; ++i) {
            const char* nm = stg[i]["name"] | "";
            uint16_t dist  = stg[i]["distance"] | 0;
            if (dist == 0) continue;
            strncpy(g_stages.targets[g_stages.count].name, nm, 16);
            g_stages.targets[g_stages.count].name[16] = '\0';
            g_stages.targets[g_stages.count].distance_yd = dist;
            g_stages.count++;
        }
    }

    // Persist to NVS
    g_storage.save(g_rifle, g_stages);
    g_config_changed = true;

    req->send(200, "text/plain", "OK");
}

/// POST /api/wifi/off — schedule WiFi shutdown.
static void handleWifiOff(AsyncWebServerRequest* req) {
    req->send(200, "text/plain", "WiFi shutting down");
    // Delay the actual shutdown so the response can be sent
    delay(500);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════

void WebServer_::begin() {
    if (active_) return;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg::WIFI_SSID, cfg::WIFI_PASS);

    if (!s_server) {
        s_server = new AsyncWebServer(cfg::WIFI_PORT);

        s_server->on("/", HTTP_GET, handleIndex);
        s_server->on("/api/config", HTTP_GET, handleGetConfig);

        // Body handler for POST /api/save
        s_server->on("/api/save", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            handleSave);

        s_server->on("/api/wifi/off", HTTP_POST, [](AsyncWebServerRequest* req) {
            handleWifiOff(req);
        });

        s_server->begin();
    }

    active_ = true;
}

void WebServer_::stop() {
    if (!active_) return;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    active_ = false;
}

void WebServer_::toggle() {
    if (active_) stop();
    else         begin();
}
