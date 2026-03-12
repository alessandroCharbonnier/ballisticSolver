/*
 * constants.h - Physical and atmospheric constants for ballistic calculations
 *
 * Based on ISA/ICAO standards.
 * Port of py_ballisticcalc.constants
 */

#pragma once

namespace bc {

// Gravity
constexpr double GRAVITY_IMPERIAL     = 32.17405;           // ft/s²
constexpr double EARTH_ANGULAR_VEL    = 7.2921159e-5;       // rad/s

// Standard atmosphere
constexpr double STD_HUMIDITY         = 0.0;                // %
constexpr double PRESSURE_EXPONENT    = 5.255876;           // g*M / (R*L)

// ISA metric
constexpr double STD_TEMP_C           = 15.0;               // °C
constexpr double LAPSE_RATE_K_PER_FT  = -0.0019812;         // K/ft
constexpr double LAPSE_RATE_METRIC    = -6.5e-03;           // °C/m
constexpr double STD_PRESSURE_HPA     = 1013.25;            // hPa
constexpr double SPEED_OF_SOUND_MET   = 20.0467;            // m/s per sqrt(K)
constexpr double STD_DENSITY_METRIC   = 1.2250;             // kg/m³

// ICAO imperial
constexpr double STD_TEMP_F           = 59.0;               // °F
constexpr double LAPSE_RATE_IMPERIAL  = -3.56616e-03;       // °F/ft
constexpr double STD_PRESSURE_INHG    = 29.92;              // inHg
constexpr double SPEED_OF_SOUND_IMP   = 49.0223;            // fps per sqrt(°R)
constexpr double STD_DENSITY_IMP      = 0.076474;           // lb/ft³

// Conversion factors
constexpr double DEG_C_TO_K           = 273.15;
constexpr double DEG_F_TO_R           = 459.67;
constexpr double DENSITY_IMP_TO_MET   = 16.0185;            // lb/ft³ → kg/m³

// Unit conversions
constexpr double INCHES_PER_FOOT      = 12.0;
constexpr double FEET_PER_YARD        = 3.0;
constexpr double INCHES_PER_YARD      = 36.0;
constexpr double FEET_PER_MILE        = 5280.0;
constexpr double MM_PER_INCH          = 25.4;
constexpr double GRAINS_PER_POUND     = 7000.0;
constexpr double FPS_PER_MPS          = 3.2808399;
constexpr double MPS_PER_FPS          = 1.0 / FPS_PER_MPS;

// Angular
constexpr double PI                   = 3.14159265358979323846;
constexpr double DEG_TO_RAD           = PI / 180.0;
constexpr double RAD_TO_DEG           = 180.0 / PI;
constexpr double MOA_TO_RAD           = PI / (60.0 * 180.0);
constexpr double MIL_TO_RAD           = PI / 3200.0;
constexpr double MRAD_TO_RAD          = 0.001;
constexpr double SMOA_TO_RAD          = 1.0 / 3600.0;       // inch/100yd → radian approx

// Ballistic computation
constexpr double MAX_WIND_DIST_FT     = 1e10;               // feet
constexpr double LOWEST_TEMP_F        = -130.0;             // °F

// Drag constant: StdDensity * PI / (4 * 2 * 144)
// = 0.076474 * 3.14159 / 1152 = 2.08551e-04
constexpr double DRAG_CONSTANT        = 2.08551e-04;

}  // namespace bc
