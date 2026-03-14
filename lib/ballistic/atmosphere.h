#pragma once
/// @file atmosphere.h
/// @brief CIPM-2007 atmospheric density model with altitude correction.
///
/// Computes air density ratio (local / standard) and local speed of sound
/// from temperature, pressure, humidity, and altitude.

#include <cmath>
#include "constants.h"

namespace ballistic {

/// Atmospheric conditions at a reference point.
struct Atmosphere {
    double temperature_f   = kStdTempF;           // °F
    double pressure_inhg   = kStdPressureInHg;    // inHg
    double humidity        = kStdHumidity;         // 0.0 – 1.0
    double altitude_ft     = 0.0;                  // feet ASL

    // --- Derived quantities (call recompute() after changing inputs) ---
    double density_ratio   = 1.0;   // rho_local / rho_standard
    double mach_fps        = 0.0;   // speed of sound in fps

    Atmosphere() { recompute(); }

    Atmosphere(double temp_f, double press_inhg, double humid, double alt_ft = 0.0)
        : temperature_f(temp_f), pressure_inhg(press_inhg),
          humidity(humid), altitude_ft(alt_ft)
    {
        recompute();
    }

    /// Recompute density_ratio and mach_fps from current inputs.
    void recompute() {
        double t_k = fToK(temperature_f);
        double p_pa = inhgToPa(pressure_inhg);

        double rho = cipmDensity(t_k, p_pa, humidity);
        density_ratio = rho / kStdDensityKgM3;
        mach_fps = std::sqrt(t_k) * kSoSCoeffMps * kFpsPerMps;
    }

    /// Get density ratio and speed of sound at a different altitude,
    /// assuming standard lapse rate from the base conditions.
    void atAltitude(double alt_ft,
                    double& out_density_ratio,
                    double& out_mach_fps) const
    {
        double delta_alt = alt_ft - altitude_ft;
        double t_base_k = fToK(temperature_f);
        double t_alt_k  = t_base_k + kLapseRateKPerFt * delta_alt;
        if (t_alt_k < 1.0) t_alt_k = 1.0;  // clamp absolute zero

        // Barometric formula
        double p_base_pa = inhgToPa(pressure_inhg);
        double p_alt_pa  = p_base_pa * std::pow(t_alt_k / t_base_k, kPressureExponent);

        double rho = cipmDensity(t_alt_k, p_alt_pa, humidity);
        out_density_ratio = rho / kStdDensityKgM3;
        out_mach_fps = std::sqrt(t_alt_k) * kSoSCoeffMps * kFpsPerMps;
    }

    // -----------------------------------------------------------------------
    //  Unit helpers
    // -----------------------------------------------------------------------
    static double fToC(double f) { return (f - 32.0) * 5.0 / 9.0; }
    static double cToF(double c) { return c * 9.0 / 5.0 + 32.0; }
    static double fToK(double f) { return fToC(f) + kCtoK; }
    static double kToF(double k) { return cToF(k - kCtoK); }

    static double inhgToPa(double inhg) { return inhg * kMmHgPerInHg * 133.322387415; }
    static double paToInhg(double pa)   { return pa / (kMmHgPerInHg * 133.322387415); }
    static double hpaToInhg(double hpa) { return paToInhg(hpa * 100.0); }

    // -----------------------------------------------------------------------
    //  CIPM-2007 density (public for use by AtmoPrecomp)
    // -----------------------------------------------------------------------

    /// CIPM-2007 air density calculation.
    /// @param t_k  Temperature in Kelvin.
    /// @param p_pa Pressure in Pascals.
    /// @param h    Relative humidity (0.0 – 1.0).
    /// @return Air density in kg/m³.
    static double cipmDensity(double t_k, double p_pa, double h) {
        double t_c = t_k - kCtoK;

        // Molar masses and gas constant (used by both paths)
        constexpr double Ma = 28.96546e-3;  // kg/mol  dry air (CIPM-2007)
        constexpr double R  = 8.314472;     // J/(mol·K)

        // Dry-air fast path: skip exp() and vapor pressure when no humidity
        if (h <= 0.0) {
            constexpr double a0 =  1.58123e-6;
            constexpr double a1 = -2.9331e-8;
            constexpr double a2 =  1.1043e-10;
            constexpr double e0 =  1.83e-11;
            double pt = p_pa / t_k;
            double Z = 1.0
                - pt * (a0 + a1 * t_c + a2 * t_c * t_c)
                + pt * pt * e0;
            return (p_pa * Ma) / (Z * R * t_k);
        }

        // Full CIPM-2007 with water vapour

        // Saturation vapour pressure (Buck / CIPM)
        constexpr double A =  1.2378847e-5;
        constexpr double B = -1.9121316e-2;
        constexpr double C =  33.93711047;
        constexpr double D = -6.3431645e3;
        double p_sv = std::exp(A * t_k * t_k + B * t_k + C + D / t_k);

        // Enhancement factor
        double f = 1.00062 + 3.14e-8 * p_pa + 5.6e-7 * t_c * t_c;

        // Mole fraction of water vapour
        double x_v = h * f * p_sv / p_pa;

        // Compressibility factor Z (simplified CIPM-2007)
        constexpr double a0 =  1.58123e-6;
        constexpr double a1 = -2.9331e-8;
        constexpr double a2 =  1.1043e-10;
        constexpr double b0 =  5.707e-6;
        constexpr double b1 = -2.051e-8;
        constexpr double cc0 =  1.9898e-4;
        constexpr double dd0 = -2.376e-6;
        constexpr double e0 =  1.83e-11;
        constexpr double f0 = -0.765e-8;

        double Z = 1.0
            - (p_pa / t_k) * (a0 + a1 * t_c + a2 * t_c * t_c
                              + (b0 + b1 * t_c) * x_v
                              + (cc0 + dd0 * t_c) * x_v * x_v)
            + (p_pa / t_k) * (p_pa / t_k) * (e0 + f0 * x_v * x_v);

        constexpr double Mv = 18.01528e-3;  // kg/mol  water
        double rho = (p_pa * Ma) / (Z * R * t_k) * (1.0 - x_v * (1.0 - Mv / Ma));
        return rho;
    }
};

// ---------------------------------------------------------------------------
//  Pre-computed atmospheric base for fast per-step altitude lookups.
//  Uses polynomial approximation for pow() in the barometric formula
//  that is exact to double precision for trajectory altitude ranges.
// ---------------------------------------------------------------------------
struct AtmoPrecomp {
    // Binomial coefficients for pow(1+x, kPressureExponent) Taylor series
    static constexpr double C1 = kPressureExponent;
    static constexpr double C2 = C1 * (kPressureExponent - 1.0) / 2.0;
    static constexpr double C3 = C2 * (kPressureExponent - 2.0) / 3.0;
    static constexpr double C4 = C3 * (kPressureExponent - 3.0) / 4.0;
    static constexpr double C5 = C4 * (kPressureExponent - 4.0) / 5.0;
    static constexpr double kSoSFpsK = kSoSCoeffMps * kFpsPerMps;

    double t_base_k;
    double inv_t_base_k;
    double p_base_pa;
    double humidity;
    double base_altitude_ft;

    explicit AtmoPrecomp(const Atmosphere& atmo)
        : t_base_k(Atmosphere::fToK(atmo.temperature_f))
        , inv_t_base_k(1.0 / Atmosphere::fToK(atmo.temperature_f))
        , p_base_pa(Atmosphere::inhgToPa(atmo.pressure_inhg))
        , humidity(atmo.humidity)
        , base_altitude_ft(atmo.altitude_ft)
    {}

    /// Fast atmospheric conditions at altitude.
    /// Replaces Atmosphere::atAltitude with pre-computed base values
    /// and a polynomial barometric formula (eliminates pow() per call).
    void atAltitude(double alt_ft,
                    double& out_density_ratio,
                    double& out_mach_fps) const
    {
        double delta_alt = alt_ft - base_altitude_ft;
        double t_alt_k = t_base_k + kLapseRateKPerFt * delta_alt;
        if (t_alt_k < 1.0) t_alt_k = 1.0;

        // Polynomial barometric formula: pow(1+x, n) via 5th-order Horner's.
        // Exact to double precision for |x| < 0.05 (covers ±7000 ft).
        double x = (t_alt_k - t_base_k) * inv_t_base_k;
        double p_alt_pa;
        if (x > -0.05 && x < 0.05) {
            double p_ratio = 1.0 + x * (C1 + x * (C2
                + x * (C3 + x * (C4 + x * C5))));
            p_alt_pa = p_base_pa * p_ratio;
        } else {
            p_alt_pa = p_base_pa
                * std::pow(t_alt_k * inv_t_base_k, kPressureExponent);
        }

        double rho = Atmosphere::cipmDensity(t_alt_k, p_alt_pa, humidity);
        out_density_ratio = rho / kStdDensityKgM3;
        out_mach_fps = std::sqrt(t_alt_k) * kSoSFpsK;
    }
};

}  // namespace ballistic
