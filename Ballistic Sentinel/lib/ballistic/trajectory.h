#pragma once
/// @file trajectory.h
/// @brief Trajectory data point and correction output types.

#include <cmath>
#include "constants.h"

namespace ballistic {

/// Correction unit preference.
enum class CorrectionUnit : uint8_t {
    MOA,    // Minute of Angle (1.047" per 100 yd)
    SMOA,   // Shooter's MOA  (1.000" per 100 yd)
    MRAD,   // Milliradians
    CM,     // Centimetres at target distance
    CLICKS  // Scope turret clicks (configurable)
};

/// A single point along the trajectory.
struct TrajectoryPoint {
    double time_s          = 0.0;   // flight time (seconds)
    double distance_ft     = 0.0;   // down-range distance (feet)
    double height_ft       = 0.0;   // height above sight line (feet)
    double windage_ft      = 0.0;   // lateral deflection (feet, + = right)
    double velocity_fps    = 0.0;   // scalar speed (fps)
    double mach            = 0.0;   // Mach number
    double energy_ftlb     = 0.0;   // kinetic energy (ft-lbs)
    double drop_angle_rad  = 0.0;   // angular correction for drop
    double wind_angle_rad  = 0.0;   // angular correction for windage
    double density_ratio   = 1.0;
};

/// The result of a single ballistic computation: correction values
/// ready for display.
struct CorrectionResult {
    double vertical       = 0.0;   // in user-selected units (+ = dial up)
    double horizontal     = 0.0;   // in user-selected units (+ = dial right)
    bool   vertical_up    = true;   // true if dial-up
    bool   horizontal_right = true; // true if dial-right
    double velocity_fps   = 0.0;
    double energy_ftlb    = 0.0;
    double time_of_flight = 0.0;
    double mach           = 0.0;
    bool   valid          = false;
};

/// Convert a raw radian angle to the requested correction unit.
/// @param rad  Angle in radians.
/// @param unit Target unit.
/// @param distance_ft  Distance to target (needed for CM output).
/// @param click_size_rad  Turret click size in radians (for CLICKS).
inline double radiansToUnit(double rad, CorrectionUnit unit,
                            double distance_ft = 0.0,
                            double click_size_rad = 0.0)
{
    switch (unit) {
        case CorrectionUnit::MOA:
            return rad * kRadToMoa;
        case CorrectionUnit::SMOA:
            // 1 SMOA = 1" per 100 yd = atan(1/3600) ≈ 1/3600 rad
            // Convert: rad / (1/3600) = rad * 3600
            return rad * 3600.0;
        case CorrectionUnit::MRAD:
            return rad * kRadToMrad;
        case CorrectionUnit::CM:
            // height at distance: tan(rad) * distance_ft, convert ft→cm
            return std::tan(rad) * distance_ft * 30.48;
        case CorrectionUnit::CLICKS:
            if (click_size_rad > 0.0) return rad / click_size_rad;
            return rad * kRadToMoa;  // fallback to MOA
        default:
            return rad * kRadToMoa;
    }
}

/// Get the short label for a correction unit.
inline const char* correctionUnitLabel(CorrectionUnit unit) {
    switch (unit) {
        case CorrectionUnit::MOA:    return "MOA";
        case CorrectionUnit::SMOA:   return "SMOA";
        case CorrectionUnit::MRAD:   return "MRAD";
        case CorrectionUnit::CM:     return "cm";
        case CorrectionUnit::CLICKS: return "CLK";
        default:                     return "?";
    }
}

/// Parse correction unit from string.
inline CorrectionUnit correctionUnitFromStr(const char* s) {
    if (!s) return CorrectionUnit::MOA;
    if (s[0] == 'M' || s[0] == 'm') {
        if (s[1] == 'O' || s[1] == 'o') return CorrectionUnit::MOA;
        if (s[1] == 'R' || s[1] == 'r') return CorrectionUnit::MRAD;
    }
    if (s[0] == 'S' || s[0] == 's') return CorrectionUnit::SMOA;
    if (s[0] == 'c' || s[0] == 'C') {
        if (s[1] == 'm' || s[1] == 'M') return CorrectionUnit::CM;
        if (s[1] == 'L' || s[1] == 'l') return CorrectionUnit::CLICKS;
    }
    return CorrectionUnit::MOA;
}

}  // namespace ballistic
