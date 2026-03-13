#include "display.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ── Fonts (all monospace) ──────────────────────────────────────────────────
// Small  : u8g2_font_5x8_mf       – 5×8   (header / sensor bar)
// Medium : u8g2_font_6x12_mf      – 6×12  (distance line)
// Large  : u8g2_font_7x14_mf      – 7×14  (corrections)
// ────────────────────────────────────────────────────────────────────────────

void Display::begin() {
    u8g2_.begin();
    u8g2_.setFontMode(1);            // transparent background
    u8g2_.setDrawColor(1);
    u8g2_.enableUTF8Print();
}

void Display::update() {
    u8g2_.clearBuffer();
    if (app_state_ == APP_STATE_MENU) {
        drawMenu();
    } else if (app_state_ == APP_STATE_SENSORS) {
        drawSensorView();
    } else if (app_state_ == APP_STATE_DIGITAL_LEVEL) {
        drawDigitalLevel();
    } else {
        drawCantSlider();
        drawHeader();
        drawDistance();
        drawCorrections();
        drawSensorBar();
    }
    u8g2_.sendBuffer();

    // Hardware-invert the display when cant is within the sensitivity
    // threshold — gives the shooter a clear "you're level" signal.
    bool level = (app_state_ == APP_STATE_LIVE || app_state_ == APP_STATE_STAGE
                  || app_state_ == APP_STATE_DIGITAL_LEVEL) &&
                 (fabsf(cant_deg_ - cant_offset_) <= cant_sensitivity_);
    if (level != inverted_) {
        inverted_ = level;
        u8x8_t* u8x8 = u8g2_.getU8x8();
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, inverted_ ? 0xA7 : 0xA6);
        u8x8_cad_EndTransfer(u8x8);
    }
}

void Display::setDimmed(bool dim) {
    if (dim == dimmed_) return;
    dimmed_ = dim;
    u8g2_.setContrast(dim ? cfg::DISPLAY_CONTRAST_DIM
                          : cfg::DISPLAY_CONTRAST_FULL);
}

// ── Data setters ───────────────────────────────────────────────────────────

void Display::setMode(bool staged, uint8_t idx, uint8_t total) {
    staged_      = staged;
    stage_idx_   = idx;
    stage_total_ = total;
}

void Display::setStageName(const char* name) {
    strncpy(stage_name_, name ? name : "", sizeof(stage_name_) - 1);
    stage_name_[sizeof(stage_name_) - 1] = '\0';
}

void Display::setDistance(uint16_t yards) { distance_yd_ = yards; }

void Display::setCorrection(const ballistic::CorrectionResult& corr,
                            ballistic::CorrectionUnit unit)
{
    v_corr_   = static_cast<float>(corr.vertical);
    v_up_     = corr.vertical_up;
    h_corr_   = static_cast<float>(corr.horizontal);
    h_right_  = corr.horizontal_right;
    strncpy(unit_label_, ballistic::correctionUnitLabel(unit), sizeof(unit_label_) - 1);
}

void Display::setSensors(float t, float p, float h) {
    temp_f_    = t;
    press_inhg_ = p;
    humidity_  = h;
}

void Display::setUnitDistance(uint8_t u) { unit_distance_ = u; }
void Display::setUnitTemperature(uint8_t u) { unit_temperature_ = u; }
void Display::setUnitPressure(uint8_t u) { unit_pressure_ = u; }
void Display::setWifiActive(bool a) { wifi_active_ = a; }
void Display::setLiveDigitCursor(uint8_t pos) { digit_cursor_ = pos; }

void Display::setAppState(uint8_t state) { app_state_ = state; }
void Display::setMenuCursor(uint8_t cursor) { menu_cursor_ = cursor; }
void Display::setWifiOn(bool on) { wifi_menu_on_ = on; }
void Display::setCant(float cant_deg) { cant_deg_ = cant_deg; }
void Display::setCantCalibration(float offset, float sensitivity) {
    cant_offset_ = offset;
    cant_sensitivity_ = sensitivity;
}

void Display::setHeading(float h) { heading_deg_ = h; }
void Display::setWind(float s, float a, bool ok) {
    wind_speed_mph_ = s; wind_angle_deg_ = a; wind_ok_ = ok;
}

void Display::showShutdown() {
    // Reset inversion so shutdown screen is always normal
    if (inverted_) {
        inverted_ = false;
        u8x8_t* u8x8 = u8g2_.getU8x8();
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, 0xA6);
        u8x8_cad_EndTransfer(u8x8);
    }
    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_6x12_mf);
    u8g2_.drawStr(16, 36, "Shutting Down");
    u8g2_.sendBuffer();
}

void Display::clearScreen() {
    if (inverted_) {
        inverted_ = false;
        u8x8_t* u8x8 = u8g2_.getU8x8();
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, 0xA6);
        u8x8_cad_EndTransfer(u8x8);
    }
    u8g2_.clearBuffer();
    u8g2_.sendBuffer();
}

void Display::showWakeProgress(uint8_t pct) {
    if (inverted_) {
        inverted_ = false;
        u8x8_t* u8x8 = u8g2_.getU8x8();
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, 0xA6);
        u8x8_cad_EndTransfer(u8x8);
    }
    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_5x8_mf);
    u8g2_.drawStr(24, 26, "Hold to wake...");
    // Progress bar: 100px wide, 8px tall, centered
    const int bx = 14, by = 34, bw = 100, bh = 8;
    u8g2_.drawFrame(bx, by, bw, bh);
    int fill = (int)((uint32_t)pct * (bw - 2) / 100);
    if (fill > 0) {
        u8g2_.drawBox(bx + 1, by + 1, fill, bh - 2);
    }
    u8g2_.sendBuffer();
}

// ── Drawing helpers ──────────────────────────────────────────────────────────

void Display::drawMenu() {
    u8g2_.setFont(u8g2_font_6x12_mf);

    constexpr uint8_t LINE_H       = 14;
    constexpr uint8_t VISIBLE_MAX  = 4;   // 64px / 14px = 4 items
    constexpr uint8_t TOTAL_ITEMS  = 5;

    // Keep scroll window centred on the cursor
    if (menu_cursor_ < menu_scroll_)
        menu_scroll_ = menu_cursor_;
    if (menu_cursor_ >= menu_scroll_ + VISIBLE_MAX)
        menu_scroll_ = menu_cursor_ - VISIBLE_MAX + 1;

    uint8_t visible = (TOTAL_ITEMS - menu_scroll_ < VISIBLE_MAX)
                    ? TOTAL_ITEMS - menu_scroll_
                    : VISIBLE_MAX;

    int y = LINE_H;
    for (uint8_t v = 0; v < visible; ++v) {
        uint8_t idx = menu_scroll_ + v;
        if (idx == menu_cursor_) {
            u8g2_.drawStr(2, y, ">");
        }
        switch (idx) {
            case 0: u8g2_.drawStr(14, y, "Live Shooting");  break;
            case 1: u8g2_.drawStr(14, y, "Stage Shooting"); break;
            case 2: u8g2_.drawStr(14, y, "Sensors");        break;
            case 3: u8g2_.drawStr(14, y, "Digital Level");   break;
            case 4:
                u8g2_.drawStr(14, y,
                    wifi_menu_on_ ? "WiFi [ON]" : "WiFi [OFF]");
                break;
        }
        y += LINE_H;
    }

    // Up arrow indicator (top-right) if items hidden above
    if (menu_scroll_ > 0) {
        drawArrowUp(118, 2);
    }
    // Down arrow indicator (bottom-right) if items hidden below
    if (menu_scroll_ + VISIBLE_MAX < TOTAL_ITEMS) {
        drawArrowDown(118, 55);
    }
}

void Display::drawSensorView() {
    u8g2_.setFont(u8g2_font_5x8_mf);
    char buf[26];
    int y = 8;

    // Temperature
    float t = temp_f_;
    const char* ts = "F";
    if (unit_temperature_ == 1) {
        t = (temp_f_ - 32.0f) * 5.0f / 9.0f;
        ts = "C";
    }
    snprintf(buf, sizeof(buf), "Temp   %6.1f %s", (double)t, ts);
    u8g2_.drawStr(0, y, buf); y += 10;

    // Pressure
    if (unit_pressure_ == 1) {
        float p = press_inhg_ * 33.8639f;
        snprintf(buf, sizeof(buf), "Press  %6.0f hPa", (double)p);
    } else {
        snprintf(buf, sizeof(buf), "Press  %5.2f inHg", (double)press_inhg_);
    }
    u8g2_.drawStr(0, y, buf); y += 10;

    // Humidity
    snprintf(buf, sizeof(buf), "Humid  %5.1f %%", (double)humidity_);
    u8g2_.drawStr(0, y, buf); y += 10;

    // Heading
    snprintf(buf, sizeof(buf), "Head   %5.1f deg", (double)heading_deg_);
    u8g2_.drawStr(0, y, buf); y += 10;

    // Cant
    snprintf(buf, sizeof(buf), "Cant   %+5.1f deg", (double)(cant_deg_ - cant_offset_));
    u8g2_.drawStr(0, y, buf); y += 10;

    // Wind
    if (wind_ok_) {
        snprintf(buf, sizeof(buf), "Wind %4.1fmph %3.0f",
                 (double)wind_speed_mph_, (double)wind_angle_deg_);
    } else {
        snprintf(buf, sizeof(buf), "Wind   --  N/A");
    }
    u8g2_.drawStr(0, y, buf);
}

void Display::drawDigitalLevel() {
    float cant = cant_deg_ - cant_offset_;
    bool is_level = fabsf(cant) <= cant_sensitivity_;

    // Header
    u8g2_.setFont(u8g2_font_5x8_mf);
    u8g2_.drawStr(0, 8, "Digital Level");

    if (is_level) {
        // ── LEVEL: big, unmistakable status ──
        u8g2_.setFont(u8g2_font_7x14_mf);
        const char* msg = "== LEVEL ==";
        int tw = u8g2_.getStrWidth(msg);
        u8g2_.drawStr((128 - tw) / 2, 38, msg);
    } else {
        // ── Off-level: show angle + direction hint ──
        char buf[16];
        snprintf(buf, sizeof(buf), "%+.1f", (double)cant);
        u8g2_.setFont(u8g2_font_7x14_mf);
        int tw = u8g2_.getStrWidth(buf);
        u8g2_.drawStr((128 - tw) / 2, 28, buf);

        u8g2_.setFont(u8g2_font_5x8_mf);
        const char* hint = (cant > 0.0f) ? "tilt LEFT" : "tilt RIGHT";
        tw = u8g2_.getStrWidth(hint);
        u8g2_.drawStr((128 - tw) / 2, 40, hint);
    }

    // ── Graphical bubble level bar ──
    const int bx = 14, bw = 100, by = 50, bh = 10;
    u8g2_.drawFrame(bx, by, bw, bh);
    // Centre tick
    u8g2_.drawVLine(bx + bw / 2, by - 2, bh + 4);
    // Sensitivity zone marks
    int zone = (int)(cant_sensitivity_ / 15.0f * (bw / 2));
    u8g2_.drawVLine(bx + bw / 2 - zone, by, bh);
    u8g2_.drawVLine(bx + bw / 2 + zone, by, bh);

    // Bubble indicator (clamp ±15°)
    float c = cant;
    if (c < -15.0f) c = -15.0f;
    if (c >  15.0f) c =  15.0f;
    int ix = bx + 1 + (int)((c + 15.0f) / 30.0f * (bw - 4));
    u8g2_.drawBox(ix, by + 1, 3, bh - 2);
}

void Display::drawCantSlider() {
    const int x0 = 56, x1 = 100, yt = 4;
    const int xc = (x0 + x1) / 2;   // 78

    // Track line
    u8g2_.drawHLine(x0, yt, x1 - x0);
    // Center tick
    u8g2_.drawVLine(xc, yt - 2, 5);

    // Map cant -15°..+15° to x0..x1 (apply calibration offset)
    float c = cant_deg_ - cant_offset_;
    if (c < -15.0f) c = -15.0f;
    if (c >  15.0f) c =  15.0f;
    int xp = xc + (int)(c * (float)(x1 - x0) / 30.0f);
    if (xp < x0) xp = x0;
    if (xp > x1) xp = x1;

    // Indicator (small filled rect)
    u8g2_.drawBox(xp - 1, yt - 2, 3, 5);
}

void Display::drawHeader() {
    u8g2_.setFont(u8g2_font_5x8_mf);

    if (staged_) {
        char buf[22];
        snprintf(buf, sizeof(buf), "STG %u/%u", stage_idx_ + 1, stage_total_);
        u8g2_.drawStr(0, 8, buf);
    } else {
        u8g2_.drawStr(0, 8, "LIVE");
    }

    // WiFi indicator
    if (wifi_active_) {
        u8g2_.drawStr(104, 8, "WiFi");
    }
}

void Display::drawDistance() {
    u8g2_.setFont(u8g2_font_6x12_mf);
    const char* du = (unit_distance_ == 1) ? "m" : "yd";

    if (staged_) {
        // Show target name + distance
        char buf[22];
        if (stage_name_[0] != '\0') {
            snprintf(buf, sizeof(buf), "%-8s %4u%s", stage_name_, distance_yd_, du);
        } else {
            snprintf(buf, sizeof(buf), "Target   %4u%s", distance_yd_, du);
        }
        u8g2_.drawStr(0, 22, buf);
    } else {
        // Live mode: draw each digit, highlight selected
        char digits[5];
        snprintf(digits, sizeof(digits), "%04u", distance_yd_ % 10000);

        int x_start = 4;
        int char_w  = 12;  // wider spacing for readability

        for (int i = 0; i < 4; ++i) {
            int x = x_start + i * char_w;
            char ch[2] = { digits[i], '\0' };

            if (i == digit_cursor_) {
                // Draw inverted (selected digit)
                u8g2_.setDrawColor(1);
                u8g2_.drawBox(x - 1, 11, char_w, 13);
                u8g2_.setDrawColor(0);
                u8g2_.drawStr(x + 2, 22, ch);
                u8g2_.setDrawColor(1);
            } else {
                u8g2_.drawStr(x + 2, 22, ch);
            }
        }

        // Unit label after digits
        u8g2_.drawStr(x_start + 4 * char_w + 8, 22, du);
    }
}

void Display::drawCorrections() {
    u8g2_.setFont(u8g2_font_7x14_mf);

    // Vertical correction
    int y_v = 38;
    if (v_up_) {
        drawArrowUp(2, y_v - 10);
    } else {
        drawArrowDown(2, y_v - 4);
    }

    char buf[20];
    snprintf(buf, sizeof(buf), "%6.2f %s", (double)v_corr_, unit_label_);
    u8g2_.drawStr(14, y_v, buf);

    // Horizontal correction
    int y_h = 54;
    if (h_right_) {
        drawArrowRight(2, y_h - 7);
    } else {
        drawArrowLeft(2, y_h - 7);
    }

    snprintf(buf, sizeof(buf), "%6.2f %s", (double)h_corr_, unit_label_);
    u8g2_.drawStr(14, y_h, buf);
}

void Display::drawSensorBar() {
    u8g2_.setFont(u8g2_font_5x8_mf);
    char buf[26];
    float t_disp = temp_f_;
    const char* t_sfx = "F";
    if (unit_temperature_ == 1) {
        t_disp = (temp_f_ - 32.0f) * 5.0f / 9.0f;
        t_sfx = "C";
    }
    if (unit_pressure_ == 1) {
        float press_hpa = press_inhg_ * 33.8639f;
        snprintf(buf, sizeof(buf), "%.0f%s %.0fhPa %.0f%%",
                 (double)t_disp, t_sfx, (double)press_hpa, (double)humidity_);
    } else {
        snprintf(buf, sizeof(buf), "%.0f%s %.1f\" %.0f%%",
                 (double)t_disp, t_sfx, (double)press_inhg_, (double)humidity_);
    }
    u8g2_.drawStr(0, 63, buf);
}

// ── Small 7×7 triangle arrow icons ────────────────────────────────────────

void Display::drawArrowUp(int x, int y) {
    u8g2_.drawTriangle(x + 4, y, x, y + 7, x + 8, y + 7);
}

void Display::drawArrowDown(int x, int y) {
    u8g2_.drawTriangle(x, y, x + 8, y, x + 4, y + 7);
}

void Display::drawArrowRight(int x, int y) {
    u8g2_.drawTriangle(x, y, x, y + 8, x + 7, y + 4);
}

void Display::drawArrowLeft(int x, int y) {
    u8g2_.drawTriangle(x + 7, y, x + 7, y + 8, x, y + 4);
}
