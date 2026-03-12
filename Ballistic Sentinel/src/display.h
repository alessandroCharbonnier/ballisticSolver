#pragma once
/// @file display.h
/// @brief OLED display manager for SH1106 128×64 I2C.

#include <U8g2lib.h>
#include <ballistic.h>
#include "config.h"

/// Display layout (128×64 monochrome, monospace fonts):
///   Y  0–9   : mode header + WiFi icon          (5×8 font)
///   Y 12–23  : distance / target info            (6×12 font)
///   Y 26–39  : vertical correction               (7×14 font)
///   Y 42–55  : horizontal correction             (7×14 font)
///   Y 58–63  : sensor bar                        (5×8 font)

class Display {
public:
    void begin();
    void update();

    // --- Data setters (called from main loop) ---
    void setMode(bool staged, uint8_t stage_idx = 0, uint8_t stage_total = 0);
    void setStageName(const char* name);
    void setDistance(uint16_t yards);
    void setCorrection(const ballistic::CorrectionResult& corr,
                       ballistic::CorrectionUnit unit);
    void setSensors(float temp_f, float press_inhg, float humidity_pct);
    void setWifiActive(bool active);
    void setLiveDigitCursor(uint8_t pos);   // 0=thousands, 1=hundreds, ...

private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_{U8G2_R0, U8X8_PIN_NONE};

    // State
    bool     staged_       = false;
    uint8_t  stage_idx_    = 0;
    uint8_t  stage_total_  = 0;
    char     stage_name_[17] = {0};
    uint16_t distance_yd_  = 0;
    bool     wifi_active_  = false;
    uint8_t  digit_cursor_ = 1;   // live mode cursor position (0-3)

    // Correction display
    float    v_corr_       = 0.0f;
    bool     v_up_         = true;
    float    h_corr_       = 0.0f;
    bool     h_right_      = true;
    char     unit_label_[6] = "MOA";

    // Sensors
    float    temp_f_       = 59.0f;
    float    press_inhg_   = 29.92f;
    float    humidity_     = 0.0f;

    void drawHeader();
    void drawDistance();
    void drawCorrections();
    void drawSensorBar();
    void drawArrowUp(int x, int y);
    void drawArrowDown(int x, int y);
    void drawArrowRight(int x, int y);
    void drawArrowLeft(int x, int y);
};
