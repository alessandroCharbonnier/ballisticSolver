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

private:
    /// CIPM-2007 air density calculation.
    /// @param t_k  Temperature in Kelvin.
    /// @param p_pa Pressure in Pascals.
    /// @param h    Relative humidity (0.0 – 1.0).
    /// @return Air density in kg/m³.
    static double cipmDensity(double t_k, double p_pa, double h) {
        double t_c = t_k - kCtoK;

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

        // Molar masses
        constexpr double Ma = 28.96546e-3;  // kg/mol  dry air (CIPM-2007)
        constexpr double Mv = 18.01528e-3;  // kg/mol  water
        constexpr double R  = 8.314472;     // J/(mol·K)

        double rho = (p_pa * Ma) / (Z * R * t_k) * (1.0 - x_v * (1.0 - Mv / Ma));
        return rho;
    }
};

}  // namespace ballistic
