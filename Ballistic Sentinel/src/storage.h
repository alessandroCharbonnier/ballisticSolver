#pragma once
/// @file storage.h
/// @brief NVS-backed persistent storage for rifle/ammo config and staged targets.

#include <cstdint>
#include <ballistic.h>

/// Maximum number of staged targets.
constexpr uint8_t MAX_STAGES = 20;

/// User-configurable rifle / ammo profile.
struct RifleConfig {
    // Drag
    float    bc                = 0.315f;
    uint8_t  drag_table        = static_cast<uint8_t>(ballistic::DragTableId::G7);
    bool     multi_bc          = false;
    float    bc_points[5]      = {0};   // up to 5 BC values
    float    bc_velocities[5]  = {0};   // corresponding velocities
    uint8_t  num_bc_points     = 0;

    // Bullet
    float    weight_gr         = 140.0f;
    float    diameter_in       = 0.264f;
    float    length_in         = 1.350f;

    // Rifle
    float    muzzle_vel_fps    = 2710.0f;
    float    sight_height_in   = 1.5f;
    float    twist_in          = 8.0f;     // positive = RH
    float    zero_range_yd     = 100.0f;

    // Powder sensitivity (optional velocity/temp table)
    bool     use_powder_sens   = false;
    float    powder_temp_f     = 59.0f;
    float    powder_modifier   = 0.0f;

    // Correction preference
    uint8_t  correction_unit   = static_cast<uint8_t>(ballistic::CorrectionUnit::MOA);
    float    click_size_moa    = 0.25f;   // turret click value in MOA

    // Coriolis
    float    latitude_deg      = 0.0f;

    // Cant calibration
    float    cant_offset       = 0.0f;  // calibration zero offset (degrees)
    float    cant_sensitivity  = 1.0f;  // threshold to signal "level" (degrees)

    // Per-unit display preferences (0 = imperial, 1 = metric)
    uint8_t  unit_distance     = 0;   // 0=yd, 1=m
    uint8_t  unit_velocity     = 0;   // 0=fps, 1=m/s
    uint8_t  unit_weight       = 0;   // 0=gr, 1=g
    uint8_t  unit_length       = 0;   // 0=in, 1=mm
    uint8_t  unit_temperature  = 0;   // 0=°F, 1=°C
    uint8_t  unit_pressure     = 0;   // 0=inHg, 1=hPa
};

/// A single staged target.
struct StageTarget {
    char     name[17]   = {0};     // max 16 chars + null
    uint16_t distance_yd = 0;
};

/// Collection of staged targets.
struct StageConfig {
    StageTarget targets[MAX_STAGES];
    uint8_t     count = 0;
};

class Storage {
public:
    /// Load config from NVS.  Returns false if no saved config found.
    bool load(RifleConfig& rifle, StageConfig& stages);

    /// Save config to NVS.
    void save(const RifleConfig& rifle, const StageConfig& stages);

    /// Erase all saved config.
    void erase();
};
