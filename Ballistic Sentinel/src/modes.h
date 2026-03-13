#pragma once
/// @file modes.h
/// @brief Main-menu, Live, and Staged shooting mode logic.

#include <ballistic.h>
#include "storage.h"
#include "input.h"
#include "sensors.h"
#include "wind.h"

enum class AppState : uint8_t {
    MAIN_MENU,
    LIVE_SHOOTING,
    STAGE_SHOOTING,
    SENSOR_VIEW
};

constexpr uint8_t MENU_ITEM_COUNT = 4;

class ModeManager {
public:
    void begin(const RifleConfig& rifle, const StageConfig& stages);

    /// Reconfigure after settings change.
    void reconfigure(const RifleConfig& rifle, const StageConfig& stages);

    /// Handle a button event.
    void handleButton(ButtonState btn);

    /// Run ballistic calculation with current sensor data.
    void compute(const SensorData& sensors, const WindData& wind);

    // --- Accessors ---
    AppState     appState()    const { return app_state_; }
    uint8_t      menuCursor()  const { return menu_cursor_; }
    bool         wifiOn()      const { return wifi_on_; }
    bool         wifiToggled()       { bool t = wifi_toggled_; wifi_toggled_ = false; return t; }
    uint16_t     distance()    const;
    uint8_t      stageIndex()  const { return stage_idx_; }
    uint8_t      stageCount()  const { return stage_count_; }
    const char*  stageName()   const;
    uint8_t      digitCursor() const { return digit_cursor_; }
    const ballistic::CorrectionResult& result() const { return result_; }
    ballistic::CorrectionUnit corrUnit() const { return corr_unit_; }
    uint16_t     displayDistance() const;
    uint8_t      unitDistance()    const { return unit_distance_; }
    uint8_t      unitTemperature() const { return unit_temperature_; }
    uint8_t      unitPressure()    const { return unit_pressure_; }

    /// Returns true if the target distance changed since last call.
    bool         distanceChanged();
    /// Returns true if config was reloaded (dirty flag, cleared on read).
    bool         configChanged();

private:
    AppState app_state_   = AppState::MAIN_MENU;
    uint8_t  menu_cursor_ = 0;
    bool     wifi_on_     = false;
    bool     wifi_toggled_ = false;

    // Live mode
    uint16_t live_distance_ = 100;
    uint8_t  digit_cursor_  = 1;

    // Staged mode
    StageTarget stages_[MAX_STAGES];
    uint8_t     stage_count_ = 0;
    uint8_t     stage_idx_   = 0;

    // Calculator
    ballistic::Calculator calc_;
    ballistic::CorrectionResult result_;
    ballistic::CorrectionUnit corr_unit_ = ballistic::CorrectionUnit::MOA;
    double click_size_rad_ = 0.0;
    float  latitude_deg_   = 0.0f;
    bool   solved_ = false;
    uint8_t unit_distance_    = 0;
    uint8_t unit_temperature_ = 0;
    uint8_t unit_pressure_    = 0;
    uint16_t prev_distance_  = 0;
    bool     distance_dirty_ = true;
    bool     config_dirty_   = true;

    void configureCaclulator(const RifleConfig& rifle);
    void adjustDigit(int delta);
    void handleMenuButton(ButtonState btn);
    void handleLiveButton(ButtonState btn);
    void handleStageButton(ButtonState btn);
    void handleSensorButton(ButtonState btn);
};
