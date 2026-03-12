#include "display.h"
#include <cstdio>
#include <cstring>

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
    } else {
        drawCantSlider();
        drawHeader();
        drawDistance();
        drawCorrections();
        drawSensorBar();
    }
    u8g2_.sendBuffer();
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

void Display::setWifiActive(bool a) { wifi_active_ = a; }
void Display::setLiveDigitCursor(uint8_t pos) { digit_cursor_ = pos; }

void Display::setAppState(uint8_t state) { app_state_ = state; }
void Display::setMenuCursor(uint8_t cursor) { menu_cursor_ = cursor; }
void Display::setWifiOn(bool on) { wifi_menu_on_ = on; }
void Display::setCant(float cant_deg) { cant_deg_ = cant_deg; }

void Display::showSleep() {
    u8g2_.clearBuffer();
    u8g2_.setFont(u8g2_font_6x12_mf);
    u8g2_.drawStr(22, 36, "Deep Sleep...");
    u8g2_.sendBuffer();
}

// ── Drawing helpers ──────────────────────────────────────────────────────────

void Display::drawMenu() {
    u8g2_.setFont(u8g2_font_6x12_mf);
    u8g2_.drawStr(34, 12, "MAIN MENU");
    u8g2_.drawHLine(0, 15, 128);

    int y = 30;
    for (uint8_t i = 0; i < 3; ++i) {
        if (i == menu_cursor_) {
            u8g2_.drawStr(2, y, ">");
        }
        switch (i) {
            case 0: u8g2_.drawStr(14, y, "Live Shooting");  break;
            case 1: u8g2_.drawStr(14, y, "Stage Shooting"); break;
            case 2:
                u8g2_.drawStr(14, y,
                    wifi_menu_on_ ? "WiFi [ON]" : "WiFi [OFF]");
                break;
        }
        y += 14;
    }
}

void Display::drawCantSlider() {
    const int x0 = 56, x1 = 100, yt = 4;
    const int xc = (x0 + x1) / 2;   // 78

    // Track line
    u8g2_.drawHLine(x0, yt, x1 - x0);
    // Center tick
    u8g2_.drawVLine(xc, yt - 2, 5);

    // Map cant -45°..+45° to x0..x1
    float c = cant_deg_;
    if (c < -45.0f) c = -45.0f;
    if (c >  45.0f) c =  45.0f;
    int xp = xc + (int)(c * (float)(x1 - x0) / 90.0f);
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

    if (staged_) {
        // Show target name + distance
        char buf[22];
        if (stage_name_[0] != '\0') {
            snprintf(buf, sizeof(buf), "%-8s %4uyd", stage_name_, distance_yd_);
        } else {
            snprintf(buf, sizeof(buf), "Target   %4uyd", distance_yd_);
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
        u8g2_.drawStr(x_start + 4 * char_w + 8, 22, "yd");
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
    snprintf(buf, sizeof(buf), "%6.1f %s", (double)v_corr_, unit_label_);
    u8g2_.drawStr(14, y_v, buf);

    // Horizontal correction
    int y_h = 54;
    if (h_right_) {
        drawArrowRight(2, y_h - 7);
    } else {
        drawArrowLeft(2, y_h - 7);
    }

    snprintf(buf, sizeof(buf), "%6.1f %s", (double)h_corr_, unit_label_);
    u8g2_.drawStr(14, y_h, buf);
}

void Display::drawSensorBar() {
    u8g2_.setFont(u8g2_font_5x8_mf);
    char buf[26];
    snprintf(buf, sizeof(buf), "%.0fF %.1f\" %.0f%%",
             (double)temp_f_, (double)press_inhg_, (double)humidity_);
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
