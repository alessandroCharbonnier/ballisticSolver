#pragma once
/// @file modes.h
/// @brief Live and Staged shooting mode logic.

#include <ballistic.h>
#include "storage.h"
#include "input.h"
#include "sensors.h"
#include "wind.h"

enum class ShootingMode : uint8_t { LIVE, STAGED };

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
    ShootingMode mode()        const { return mode_; }
    uint16_t     distance()    const;
    uint8_t      stageIndex()  const { return stage_idx_; }
    uint8_t      stageCount()  const { return stage_count_; }
    const char*  stageName()   const;
    uint8_t      digitCursor() const { return digit_cursor_; }
    const ballistic::CorrectionResult& result() const { return result_; }
    ballistic::CorrectionUnit corrUnit() const { return corr_unit_; }

private:
    ShootingMode mode_ = ShootingMode::LIVE;

    // Live mode
    uint16_t live_distance_ = 100;   // yards
    uint8_t  digit_cursor_  = 1;     // 0=thousands, 1=hundreds, 2=tens, 3=ones

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

    void configureCaclulator(const RifleConfig& rifle);
    void adjustDigit(int delta);
};
