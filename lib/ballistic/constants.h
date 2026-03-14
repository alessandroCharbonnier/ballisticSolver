#pragma once
/// @file constants.h
/// @brief Physical constants for external ballistics (matching py_ballisticcalc).

namespace ballistic {

// --- Gravity ---
constexpr double kGravityFps2        = -32.17405;         // ft/s² (negative = downward)

// --- Standard atmosphere (ICAO / Army Standard Metro) ---
constexpr double kStdTempF           = 59.0;              // °F  (15 °C)
constexpr double kStdPressureInHg    = 29.92;             // inHg
constexpr double kStdPressureHPa     = 1013.25;           // hPa
constexpr double kStdDensityLbFt3    = 0.076474;          // lb/ft³
constexpr double kStdDensityKgM3     = 1.2250;            // kg/m³
constexpr double kStdHumidity        = 0.0;               // 0-1

// --- Temperature lapse rate ---
constexpr double kLapseRateKPerFt    = -0.0019812;        // K per foot
constexpr double kLapseRateKPerM     = -0.0065;           // K per metre

// --- Pressure altitude exponent ---
constexpr double kPressureExponent   = 5.255876;

// --- Speed of sound coefficients ---
constexpr double kSoSCoeffFps        = 49.0223;           // sqrt(°R) → fps
constexpr double kSoSCoeffMps        = 20.0467;           // sqrt(K)  → m/s

// --- Temperature conversions ---
constexpr double kCtoK               = 273.15;
constexpr double kFtoR               = 459.67;

// --- Coriolis ---
constexpr double kEarthOmegaRadS     = 7.2921159e-5;      // rad/s

// --- Drag factor constant ---
// rho_std_lb_ft3 * pi / (4 * 2 * 144)  ≈ 2.08551e-04
constexpr double kDragConvFactor     = 2.08551e-04;

// --- Trigonometry ---
constexpr double kPi                 = 3.14159265358979323846;
constexpr double kDegToRad           = kPi / 180.0;
constexpr double kRadToDeg           = 180.0 / kPi;

// --- Unit conversions ---
constexpr double kInchesPerFoot      = 12.0;
constexpr double kFeetPerYard        = 3.0;
constexpr double kInchesPerYard      = 36.0;
constexpr double kFeetPerMetre       = 3.2808399;
constexpr double kFpsPerMps          = 3.2808399;
constexpr double kMpsPerFps          = 1.0 / kFpsPerMps;
constexpr double kGrainsPerPound     = 7000.0;
constexpr double kMmHgPerInHg        = 25.4;

// --- Angular unit conversions (to radians) ---
constexpr double kMoaToRad           = kPi / 10800.0;
constexpr double kRadToMoa           = 10800.0 / kPi;
constexpr double kMradToRad          = 0.001;
constexpr double kRadToMrad          = 1000.0;
constexpr double kSmoaToRad          = kPi / (180.0 * 60.0);  // same as MOA for this
// Shooter's MOA (SMOA) = 1.047" at 100 yd vs true MOA = 1.0472"
// SMOA = atan(1/3600) ≈ 1/3600 rad conceptually, but commonly:
// 1 SMOA ≈ 1" per 100 yd = 1/3600 rad (practical approximation)
constexpr double kSmoaInchPer100Yd   = 1.0;               // exactly 1" at 100 yd
constexpr double kMoaInchPer100Yd    = 1.04720;            // 1.047" at 100 yd

// --- Zero-finding ---
constexpr double kZeroFindAccuracy   = 0.000005;           // feet
constexpr int    kMaxZeroIterations  = 40;
constexpr double kMinAltitude        = -1500.0;            // feet
constexpr double kMaxDrop            = -10000.0;           // feet
constexpr double kMinVelocity        = 50.0;               // fps

}  // namespace ballistic
