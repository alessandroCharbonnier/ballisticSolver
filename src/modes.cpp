#include "modes.h"
#include <cmath>
#include <cstring>

// ═══════════════════════════════════════════════════════════════════════════

void ModeManager::begin(const RifleConfig& rifle, const StageConfig& stages) {
    reconfigure(rifle, stages);
}

void ModeManager::reconfigure(const RifleConfig& rifle, const StageConfig& stages) {
    // Copy stage targets
    stage_count_ = stages.count;
    for (uint8_t i = 0; i < stage_count_; ++i) {
        stages_[i] = stages.targets[i];
    }
    if (stage_idx_ >= stage_count_ && stage_count_ > 0) {
        stage_idx_ = 0;
    }

    corr_unit_ = static_cast<ballistic::CorrectionUnit>(rifle.correction_unit);
    latitude_deg_ = rifle.latitude_deg;
    unit_distance_    = rifle.unit_distance;
    unit_temperature_ = rifle.unit_temperature;
    unit_pressure_    = rifle.unit_pressure;
    config_dirty_ = true;
    distance_dirty_ = true;

    // Click size: convert MOA to radians
    click_size_rad_ = rifle.click_size_moa * ballistic::kMoaToRad;

    configureCaclulator(rifle);
}

void ModeManager::configureCaclulator(const RifleConfig& rifle) {
    auto table = static_cast<ballistic::DragTableId>(rifle.drag_table);

    if (rifle.multi_bc && rifle.num_bc_points > 0) {
        ballistic::BCPoint pts[5];
        uint8_t n = rifle.num_bc_points;
        if (n > 5) n = 5;
        for (uint8_t i = 0; i < n; ++i) {
            pts[i].bc = rifle.bc_points[i];
            pts[i].velocity_fps = rifle.bc_velocities[i];
        }
        calc_.configureMultiBC(pts, n, table,
            rifle.weight_gr, rifle.diameter_in, rifle.length_in,
            rifle.muzzle_vel_fps, rifle.sight_height_in,
            rifle.twist_in, rifle.zero_range_yd);
    } else {
        calc_.configure(
            rifle.bc, table,
            rifle.weight_gr, rifle.diameter_in, rifle.length_in,
            rifle.muzzle_vel_fps, rifle.sight_height_in,
            rifle.twist_in, rifle.zero_range_yd);
    }

    if (rifle.use_powder_sens && rifle.powder_modifier != 0.0f) {
        calc_.setPowderSensitivity(rifle.powder_temp_f, rifle.powder_modifier);
    }

    solved_ = false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Button handling
// ═══════════════════════════════════════════════════════════════════════════

void ModeManager::handleButton(ButtonState btn) {
    if (btn.event == ButtonEvent::NONE) return;

    // LONG_PRESS CENTER → deep sleep (handled by main.cpp before this)
    if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::LONG_PRESS)
        return;

    switch (app_state_) {
        case AppState::MAIN_MENU:      handleMenuButton(btn);  break;
        case AppState::LIVE_SHOOTING:  handleLiveButton(btn);  break;
        case AppState::STAGE_SHOOTING: handleStageButton(btn); break;
        case AppState::SENSOR_VIEW:    handleSensorButton(btn); break;
        case AppState::DIGITAL_LEVEL:  handleDigitalLevelButton(btn); break;
    }
}

void ModeManager::handleMenuButton(ButtonState btn) {
    if (btn.id == ButtonId::UP
        && (btn.event == ButtonEvent::PRESS || btn.event == ButtonEvent::REPEAT))
    {
        if (menu_cursor_ > 0) menu_cursor_--;
    }
    else if (btn.id == ButtonId::DOWN
             && (btn.event == ButtonEvent::PRESS || btn.event == ButtonEvent::REPEAT))
    {
        if (menu_cursor_ < MENU_ITEM_COUNT - 1) menu_cursor_++;
    }
    else if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::PRESS) {
        switch (menu_cursor_) {
            case 0: app_state_ = AppState::LIVE_SHOOTING;  break;
            case 1: app_state_ = AppState::STAGE_SHOOTING; break;
            case 2: app_state_ = AppState::DIGITAL_LEVEL;  break;
            case 3: app_state_ = AppState::SENSOR_VIEW;    break;
            case 4:
                wifi_on_ = !wifi_on_;
                wifi_toggled_ = true;
                break;
        }
    }
}

void ModeManager::handleLiveButton(ButtonState btn) {
    // Double-press CENTER → back to menu
    if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::DOUBLE_PRESS) {
        app_state_ = AppState::MAIN_MENU;
        return;
    }

    if (btn.event != ButtonEvent::PRESS && btn.event != ButtonEvent::REPEAT)
        return;

    switch (btn.id) {
        case ButtonId::UP:    adjustDigit(+1); break;
        case ButtonId::DOWN:  adjustDigit(-1); break;
        case ButtonId::LEFT:
            if (digit_cursor_ > 0) digit_cursor_--;
            break;
        case ButtonId::RIGHT:
            if (digit_cursor_ < 3) digit_cursor_++;
            break;
        default: break;
    }
}

void ModeManager::handleStageButton(ButtonState btn) {
    // Double-press CENTER → back to menu
    if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::DOUBLE_PRESS) {
        app_state_ = AppState::MAIN_MENU;
        return;
    }

    if (btn.event != ButtonEvent::PRESS && btn.event != ButtonEvent::REPEAT)
        return;

    switch (btn.id) {
        case ButtonId::RIGHT:
            if (stage_count_ > 0) {
                stage_idx_ = (stage_idx_ + 1) % stage_count_;
                distance_dirty_ = true;
            }
            break;
        case ButtonId::LEFT:
            if (stage_count_ > 0) {
                stage_idx_ = (stage_idx_ == 0) ? stage_count_ - 1 : stage_idx_ - 1;
                distance_dirty_ = true;
            }
            break;
        default: break;
    }
}

void ModeManager::handleSensorButton(ButtonState btn) {
    // Double-press CENTER → back to menu
    if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::DOUBLE_PRESS) {
        app_state_ = AppState::MAIN_MENU;
    }
}

void ModeManager::handleDigitalLevelButton(ButtonState btn) {
    // Double-press CENTER → back to menu
    if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::DOUBLE_PRESS) {
        app_state_ = AppState::MAIN_MENU;
    }
}

void ModeManager::adjustDigit(int delta) {
    // Decompose distance into digits [thousands, hundreds, tens, ones]
    int d = live_distance_;
    int digits[4] = {
        (d / 1000) % 10,
        (d / 100)  % 10,
        (d / 10)   % 10,
        d % 10
    };

    // Wrap around (combination-lock style)
    digits[digit_cursor_] = (digits[digit_cursor_] + delta + 10) % 10;

    live_distance_ = static_cast<uint16_t>(
        digits[0] * 1000 + digits[1] * 100 + digits[2] * 10 + digits[3]);
    distance_dirty_ = true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Ballistic computation
// ═══════════════════════════════════════════════════════════════════════════

void ModeManager::compute(const SensorData& sensors, const WindData& wind) {
    // Update atmosphere from live sensor data
    calc_.setAtmosphere(
        sensors.temperature_f,
        sensors.pressure_inhg,
        sensors.humidity_pct / 100.0,
        0.0   // altitude ASL — could be derived from pressure
    );

    // Wind (if Calypso is available)
    if (wind.available) {
        // Calypso angle: 0 = front (headwind), relative to rifle
        // Convert mph → fps
        double speed_fps = wind.speed_mph * 5280.0 / 3600.0;
        // Calypso 0° = headwind → our convention 180° = headwind
        // Calypso 90° = from right → our convention 270° (from right, push left)
        // Mapping: our_deg = (wind.angle + 180) % 360
        double angle_deg = fmod(wind.angle_deg + 180.0, 360.0);
        calc_.setWind(speed_fps, angle_deg);
    } else {
        calc_.setWind(0.0, 0.0);
    }

    // Coriolis (use compass heading as azimuth)
    // TODO: Re-enable once magnetometer is properly calibrated / shielded
    // if (std::abs(latitude_deg_) > 0.1f) {
    //     calc_.setCoriolis(latitude_deg_, sensors.heading_deg);
    // } else {
    //     calc_.disableCoriolis();
    // }
    calc_.disableCoriolis();

    // Find zero (only re-solve when atmosphere changes significantly)
    if (!solved_ || calc_.isDirty()) {
        calc_.solve();
        solved_ = true;
    }

    // Compute correction at target distance
    uint16_t dist = distance();
    if (dist > 0) {
        result_ = calc_.correction(dist, corr_unit_, click_size_rad_);
    } else {
        result_ = ballistic::CorrectionResult{};
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Accessors
// ═══════════════════════════════════════════════════════════════════════════

uint16_t ModeManager::distance() const {
    if (app_state_ == AppState::LIVE_SHOOTING) {
        if (unit_distance_ == 1) {
            return static_cast<uint16_t>(live_distance_ / 0.9144 + 0.5);
        }
        return live_distance_;
    }
    if (app_state_ == AppState::STAGE_SHOOTING
        && stage_count_ > 0 && stage_idx_ < stage_count_)
    {
        return stages_[stage_idx_].distance_yd;
    }
    return 0;
}

bool ModeManager::distanceChanged() {
    uint16_t cur = distance();
    if (cur != prev_distance_ || distance_dirty_) {
        prev_distance_ = cur;
        distance_dirty_ = false;
        return true;
    }
    return false;
}

bool ModeManager::configChanged() {
    bool c = config_dirty_;
    config_dirty_ = false;
    return c;
}

uint16_t ModeManager::displayDistance() const {
    if (app_state_ == AppState::LIVE_SHOOTING) {
        return live_distance_;
    }
    if (app_state_ == AppState::STAGE_SHOOTING
        && stage_count_ > 0 && stage_idx_ < stage_count_)
    {
        if (unit_distance_ == 1) {
            return static_cast<uint16_t>(stages_[stage_idx_].distance_yd * 0.9144 + 0.5);
        }
        return stages_[stage_idx_].distance_yd;
    }
    return 0;
}

const char* ModeManager::stageName() const {
    if (stage_count_ > 0 && stage_idx_ < stage_count_) {
        return stages_[stage_idx_].name;
    }
    return "";
}
