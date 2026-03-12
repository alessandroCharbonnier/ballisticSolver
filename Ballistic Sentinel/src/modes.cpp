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

    // CENTER short → toggle mode
    if (btn.id == ButtonId::CENTER && btn.event == ButtonEvent::PRESS) {
        mode_ = (mode_ == ShootingMode::LIVE) ? ShootingMode::STAGED
                                               : ShootingMode::LIVE;
        return;
    }

    // CENTER long → handled by main (WiFi toggle)
    if (btn.id == ButtonId::CENTER) return;

    // Only PRESS and REPEAT for directional buttons
    if (btn.event != ButtonEvent::PRESS && btn.event != ButtonEvent::REPEAT)
        return;

    if (mode_ == ShootingMode::LIVE) {
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
    } else {
        // Staged mode: left/right cycle targets
        switch (btn.id) {
            case ButtonId::RIGHT:
                if (stage_count_ > 0) {
                    stage_idx_ = (stage_idx_ + 1) % stage_count_;
                }
                break;
            case ButtonId::LEFT:
                if (stage_count_ > 0) {
                    stage_idx_ = (stage_idx_ == 0)
                        ? stage_count_ - 1 : stage_idx_ - 1;
                }
                break;
            default: break;  // UP/DOWN do nothing in staged mode
        }
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
    if (std::abs(latitude_deg_) > 0.1f) {
        calc_.setCoriolis(latitude_deg_, sensors.heading_deg);
    } else {
        calc_.disableCoriolis();
    }

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
    if (mode_ == ShootingMode::LIVE) {
        return live_distance_;
    }
    if (stage_count_ > 0 && stage_idx_ < stage_count_) {
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
