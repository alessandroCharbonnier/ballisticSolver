#pragma once
/// @file engine_rk4.h
/// @brief 4th-order Runge-Kutta trajectory integrator with zero finder.
///
/// Faithfully ports the RK4 engine from py_ballisticcalc.  All internal
/// computation is in feet / seconds / radians / grains.

#include <cmath>
#include <vector>

#include "constants.h"
#include "vector3d.h"
#include "atmosphere.h"
#include "drag_model.h"
#include "trajectory.h"

namespace ballistic {

/// Wind specification.
struct Wind {
    double velocity_fps  = 0.0;
    double direction_rad = 0.0;   // 0 = tailwind, pi/2 = from left, pi = headwind

    /// Decompose into down-range (x) and cross-range (z) components.
    Vector3 vector() const {
        return {
            velocity_fps * std::cos(direction_rad),   // x: tail(+) / head(−)
            0.0,
            velocity_fps * std::sin(direction_rad)    // z: from-left push right(+)
        };
    }
};

/// Coriolis helper (full 3-D model).
struct CoriolisParams {
    bool   enabled    = false;
    double latitude   = 0.0;     // radians
    double azimuth    = 0.0;     // radians (compass bearing of shot)

    // Pre-computed direction cosines
    double cos_lat    = 1.0;
    double sin_lat    = 0.0;
    double cos_az     = 1.0;
    double sin_az     = 0.0;

    void precompute() {
        cos_lat = std::cos(latitude);
        sin_lat = std::sin(latitude);
        cos_az  = std::cos(azimuth);
        sin_az  = std::sin(azimuth);
    }

    /// Coriolis acceleration for a given velocity vector.
    Vector3 acceleration(const Vector3& vel) const {
        if (!enabled) return kZeroVector;
        double omega2 = 2.0 * kEarthOmegaRadS;
        // In ENU-like frame → project to range/up/cross
        double ax = -vel.y * cos_lat * sin_az - vel.z * sin_lat;
        double ay =  vel.x * cos_lat * sin_az + vel.z * cos_lat * cos_az;
        double az =  vel.x * sin_lat          - vel.y * cos_lat * cos_az;
        return { omega2 * ax, omega2 * ay, omega2 * az };
    }
};

/// Complete shot configuration for the engine.
struct ShotConfig {
    // Projectile / rifle
    DragModel    drag;
    double       muzzle_vel_fps   = 0.0;
    double       sight_height_ft  = 0.0;     // above bore (feet)
    double       twist_in         = 0.0;     // barrel twist (inches, + = RH)

    // Zeroing
    double       zero_distance_ft = 0.0;
    double       zero_elevation   = 0.0;     // radians (computed by zero finder)

    // Atmosphere
    Atmosphere   atmo;

    // Wind
    Wind         wind;

    // Coriolis
    CoriolisParams coriolis;

    // Angles
    double       look_angle_rad   = 0.0;     // incline angle (+ = uphill)
    double       cant_angle_rad   = 0.0;

    // Powder sensitivity
    double       powder_temp_f    = kStdTempF;
    double       temp_modifier    = 0.0;     // fps per °F per (15/v0)
    bool         use_temp_sens    = false;

    /// Get muzzle velocity adjusted for current temperature.
    double effectiveMuzzleVel() const {
        if (!use_temp_sens || temp_modifier == 0.0) return muzzle_vel_fps;
        double v0 = muzzle_vel_fps;
        double ratio = temp_modifier / (15.0 / v0);
        return ratio * (atmo.temperature_f - powder_temp_f) + v0;
    }

    /// Miller stability coefficient (matches py_ballisticcalc).
    double stabilityCoefficient() const {
        double w = drag.weight_gr;
        double d = drag.diameter_in;
        double l = drag.length_in;
        double t = std::abs(twist_in);
        if (t <= 0.0 || d <= 0.0 || l <= 0.0) return 1.5; // safe default
        if (atmo.pressure_inhg <= 0.0) return 1.5;

        // Normalize twist and length to calibers
        double twist_cal  = t / d;
        double length_cal = l / d;

        // Miller stability formula
        double sd = 30.0 * w /
            (twist_cal * twist_cal * d * d * d *
             length_cal * (1.0 + length_cal * length_cal));

        // Velocity correction factor
        double fv = std::pow(muzzle_vel_fps / 2800.0, 1.0 / 3.0);

        // Atmospheric correction (temperature + pressure)
        double ftp = ((atmo.temperature_f + 460.0) / (59.0 + 460.0))
                   * (29.92 / atmo.pressure_inhg);

        return sd * fv * ftp;
    }

    /// Spin drift at a given time of flight (Litz formula).
    /// @return drift in feet (positive = right for RH twist).
    double spinDrift(double time_s) const {
        if (twist_in == 0.0) return 0.0;
        double sg = stabilityCoefficient();
        double sign = (twist_in > 0.0) ? 1.0 : -1.0;
        // 1.25 * (Sg + 1.2) * t^1.83  gives inches → convert to feet
        return sign * (1.25 * (sg + 1.2) * std::pow(time_s, 1.83)) / 12.0;
    }

    /// Spin drift with pre-computed stability coefficient (avoids
    /// redundant pow()/sqrt() calls when used in integration loops).
    double spinDrift(double time_s, double sg) const {
        if (twist_in == 0.0) return 0.0;
        double sign = (twist_in > 0.0) ? 1.0 : -1.0;
        return sign * (1.25 * (sg + 1.2) * std::pow(time_s, 1.83)) / 12.0;
    }
};

// ===========================================================================
//  RK4 Integration Engine
// ===========================================================================

class EngineRK4 {
public:
    // --- Adaptive RK45 Dormand-Prince step size control ---
    static constexpr double kDefaultDt     = 0.005;    // initial step (seconds)
    static constexpr double kMinDt         = 0.0005;   // minimum step size
    static constexpr double kMaxDt         = 0.02;     // maximum step size
    static constexpr double kRK45Tol       = 1e-6;     // local truncation error tolerance (feet)
    static constexpr double kSafetyFactor  = 0.84;     // step growth safety factor
    static constexpr int    kMaxZeroIter   = 40;
    static constexpr double kZeroAccFt     = 0.000005;
    static constexpr double kMinVelocitySq = kMinVelocity * kMinVelocity;

    // --- Atmosphere amortization ---
    static constexpr int    kAtmoSkipSteps = 8;        // recalculate atmosphere every N steps

    // --- Dormand-Prince RK45 coefficients ---
    // Butcher tableau constants (static constexpr for zero runtime cost)
    static constexpr double a21 = 1.0/5.0;
    static constexpr double a31 = 3.0/40.0,     a32 = 9.0/40.0;
    static constexpr double a41 = 44.0/45.0,    a42 = -56.0/15.0,  a43 = 32.0/9.0;
    static constexpr double a51 = 19372.0/6561.0, a52 = -25360.0/2187.0, a53 = 64448.0/6561.0, a54 = -212.0/729.0;
    static constexpr double a61 = 9017.0/3168.0, a62 = -355.0/33.0, a63 = 46732.0/5247.0, a64 = 49.0/176.0, a65 = -5103.0/18656.0;
    // 5th-order weights
    static constexpr double b1 = 35.0/384.0, b3 = 500.0/1113.0, b4 = 125.0/192.0, b5 = -2187.0/6784.0, b6 = 11.0/84.0;
    // 4th-order weights (for error estimate)
    static constexpr double e1 = 71.0/57600.0, e3 = -71.0/16695.0, e4 = 71.0/1920.0, e5 = -17253.0/339200.0, e6 = 22.0/525.0, e7 = -1.0/40.0;

    /// Find the zero elevation (barrel angle that zeros at zero_distance).
    /// Modifies shot.zero_elevation in-place.
    /// @return true on success.
    bool findZero(ShotConfig& shot) const {
        if (shot.zero_distance_ft <= 0.0) {
            shot.zero_elevation = 0.0;
            return true;
        }

        // Initial guess: flat-fire approximation
        // drop ≈ 0.5 * g * t² where t ≈ dist / v0
        double t_est = shot.zero_distance_ft / shot.effectiveMuzzleVel();
        double drop_est = 0.5 * std::abs(kGravityFps2) * t_est * t_est;
        double elev = std::atan2(drop_est + shot.sight_height_ft,
                                 shot.zero_distance_ft);

        // Iterative refinement
        double lo = 0.0, hi = 0.0;
        bool have_lo = false, have_hi = false;

        for (int iter = 0; iter < kMaxZeroIter; ++iter) {
            shot.zero_elevation = elev;
            double height = traceToDistance(shot, shot.zero_distance_ft);

            if (std::abs(height) < kZeroAccFt) return true;

            // Sensitivity: dh/dθ ≈ dist / cos²(θ)
            double sensitivity = shot.zero_distance_ft /
                std::cos(elev) / std::cos(elev);
            // Newton step: Δθ = -height / sensitivity  (height>0 → decrease θ)
            double correction = -height / sensitivity;

            // Damping for stability
            double damp = (iter < 5) ? 0.8 : 0.5;
            double new_elev = elev + correction * damp;

            // Track bracket for Ridder's fallback
            if (height > 0.0) {
                hi = elev; have_hi = true;
            } else {
                lo = elev; have_lo = true;
            }

            elev = new_elev;
        }

        // Ridder's method fallback if bracket found
        if (have_lo && have_hi) {
            return ridderZero(shot, lo, hi);
        }

        shot.zero_elevation = elev;
        return false;  // did not converge
    }

    /// Run full trajectory and get a single correction at target_distance.
    CorrectionResult computeCorrection(
        const ShotConfig& shot,
        double target_distance_ft,
        CorrectionUnit unit,
        double click_size_rad = 0.0) const
    {
        CorrectionResult result;
        result.valid = false;

        if (target_distance_ft <= 0.0) return result;

        TrajectoryPoint pt = traceToPoint(shot, target_distance_ft);

        if (pt.distance_ft < target_distance_ft * 0.9) {
            return result;  // bullet didn't reach target
        }

        // Corrections relative to sight line
        double slant_height = pt.height_ft * std::cos(shot.look_angle_rad)
                            - pt.distance_ft * std::sin(shot.look_angle_rad);
        double slant_dist   = pt.distance_ft * std::cos(shot.look_angle_rad)
                            + pt.height_ft * std::sin(shot.look_angle_rad);

        if (slant_dist <= 0.0) return result;

        // Angular corrections
        double drop_rad    = std::atan2(-slant_height, slant_dist);  // neg height → positive correction
        double windage_rad = std::atan2(pt.windage_ft, slant_dist);

        // Convert to user units
        double v_corr = radiansToUnit(std::abs(drop_rad), unit,
                                       pt.distance_ft, click_size_rad);
        double h_corr = radiansToUnit(std::abs(windage_rad), unit,
                                       pt.distance_ft, click_size_rad);

        result.vertical       = v_corr;
        result.horizontal     = h_corr;
        result.vertical_up    = (drop_rad >= 0.0);  // drop_rad > 0 means bullet is low → dial up
        result.horizontal_right = (windage_rad >= 0.0);
        result.velocity_fps   = pt.velocity_fps;
        result.energy_ftlb    = pt.energy_ftlb;
        result.time_of_flight = pt.time_s;
        result.mach           = pt.mach;
        result.valid          = true;

        return result;
    }

    /// Run full trajectory returning points at each step_ft.
    std::vector<TrajectoryPoint> trajectory(
        const ShotConfig& shot,
        double max_distance_ft,
        double step_ft) const
    {
        std::vector<TrajectoryPoint> points;
        if (max_distance_ft <= 0.0 || step_ft <= 0.0) return points;

        double mv = shot.effectiveMuzzleVel();
        double barrel_elev = shot.look_angle_rad + shot.zero_elevation;
        double barrel_az   = 0.0;
        if (shot.cant_angle_rad != 0.0) {
            barrel_elev = shot.look_angle_rad
                        + std::cos(shot.cant_angle_rad) * shot.zero_elevation;
            barrel_az   = std::sin(shot.cant_angle_rad) * shot.zero_elevation;
        }

        // Initial conditions
        Vector3 pos = {0.0, -shot.sight_height_ft, 0.0};
        Vector3 vel = {
            mv * std::cos(barrel_elev) * std::cos(barrel_az),
            mv * std::sin(barrel_elev),
            mv * std::cos(barrel_elev) * std::sin(barrel_az)
        };

        Vector3 wind_vec = shot.wind.vector();
        double  next_step = step_ft;
        double  time = 0.0;
        double  dt = kDefaultDt;

        // Pre-compute for performance
        AtmoPrecomp atmo_pre(shot.atmo);
        double drag_coeff = (shot.drag.bc > 0.0)
            ? kDragConvFactor / shot.drag.bc : 0.0;
        size_t spline_hint = shot.drag.spline.n / 2;

        // Atmosphere amortization state
        double density_ratio, mach_local, inv_mach;
        atmo_pre.atAltitude(shot.atmo.altitude_ft + pos.y,
                            density_ratio, mach_local);
        inv_mach = 1.0 / mach_local;
        int atmo_counter = 0;

        // Record initial point
        recordPoint(points, shot, pos, vel, time, wind_vec);

        while (pos.x < max_distance_ft) {
            // Refresh atmosphere periodically
            if (atmo_counter <= 0) {
                atmo_pre.atAltitude(shot.atmo.altitude_ft + pos.y,
                                    density_ratio, mach_local);
                inv_mach = 1.0 / mach_local;
                atmo_counter = kAtmoSkipSteps;
            }
            --atmo_counter;

            double dr = density_ratio;
            double im = inv_mach;

            // --- RK45 Dormand-Prince step ---
            auto accel = [&](const Vector3& p, const Vector3& v) -> Vector3 {
                Vector3 v_rel = v - wind_vec;
                double  speed = v_rel.fastMagnitude();

                Vector3 a = {0.0, kGravityFps2, 0.0};  // gravity
                if (speed > 0.0) {
                    double mach = speed * im;
                    double drag = dr
                        * shot.drag.spline.evalHinted(mach, spline_hint)
                        * drag_coeff;
                    a -= v_rel * (drag * speed);  // drag deceleration
                }
                a += shot.coriolis.acceleration(v);  // Coriolis

                return a;
            };

            Vector3 k1 = accel(pos, vel);
            Vector3 k2 = accel(pos, vel + k1 * (dt * a21));
            Vector3 k3 = accel(pos, vel + k1 * (dt * a31) + k2 * (dt * a32));
            Vector3 k4 = accel(pos, vel + k1 * (dt * a41) + k2 * (dt * a42) + k3 * (dt * a43));
            Vector3 k5 = accel(pos, vel + k1 * (dt * a51) + k2 * (dt * a52) + k3 * (dt * a53) + k4 * (dt * a54));
            Vector3 k6 = accel(pos, vel + k1 * (dt * a61) + k2 * (dt * a62) + k3 * (dt * a63) + k4 * (dt * a64) + k5 * (dt * a65));

            Vector3 vel_new = vel + (k1 * b1 + k3 * b3 + k4 * b4 + k5 * b5 + k6 * b6) * dt;

            // Error estimate
            Vector3 k7 = accel(pos, vel_new);
            Vector3 err_vec = (k1 * e1 + k3 * e3 + k4 * e4 + k5 * e5 + k6 * e6 + k7 * e7) * dt;
            double err = err_vec.fastMagnitude();

            if (err > kRK45Tol && dt > kMinDt) {
                double scale = kSafetyFactor * std::pow(kRK45Tol / err, 0.25);
                dt *= (scale > 0.2) ? scale : 0.2;
                if (dt < kMinDt) dt = kMinDt;
                continue;
            }

            // Accept step
            pos += (vel + vel_new) * (dt * 0.5);
            vel = vel_new;
            time += dt;

            // Adapt step size
            if (err > 0.0) {
                double scale = kSafetyFactor * std::pow(kRK45Tol / err, 0.2);
                if (scale > 4.0) scale = 4.0;
                dt *= scale;
            } else {
                dt *= 2.0;
            }
            if (dt > kMaxDt) dt = kMaxDt;
            if (dt < kMinDt) dt = kMinDt;

            // Record point at each distance step
            if (pos.x >= next_step) {
                recordPoint(points, shot, pos, vel, time, wind_vec);
                next_step += step_ft;
            }

            // Termination conditions
            if (vel.magnitudeSquared() < kMinVelocitySq) break;
            if (pos.y < kMaxDrop) break;
            if (pos.y < kMinAltitude) break;
        }

        return points;
    }

private:
    /// Integrate trajectory to a specific distance, return height at that point.
    /// Used by zero finder.  Uses adaptive RK45 Dormand-Prince.
    double traceToDistance(const ShotConfig& shot, double target_dist_ft) const {
        double mv = shot.effectiveMuzzleVel();
        double barrel_elev = shot.look_angle_rad + shot.zero_elevation;

        Vector3 pos = {0.0, -shot.sight_height_ft, 0.0};
        Vector3 vel = {
            mv * std::cos(barrel_elev),
            mv * std::sin(barrel_elev),
            0.0
        };

        Vector3 wind_vec = shot.wind.vector();
        double  dt = kDefaultDt;

        // Pre-compute for performance
        AtmoPrecomp atmo_pre(shot.atmo);
        double drag_coeff = (shot.drag.bc > 0.0)
            ? kDragConvFactor / shot.drag.bc : 0.0;
        size_t spline_hint = shot.drag.spline.n / 2;

        // Atmosphere amortization state
        double density_ratio, mach_local, inv_mach;
        atmo_pre.atAltitude(shot.atmo.altitude_ft + pos.y,
                            density_ratio, mach_local);
        inv_mach = 1.0 / mach_local;
        int atmo_counter = 0;

        while (pos.x < target_dist_ft) {
            // Refresh atmosphere periodically
            if (atmo_counter <= 0) {
                atmo_pre.atAltitude(shot.atmo.altitude_ft + pos.y,
                                    density_ratio, mach_local);
                inv_mach = 1.0 / mach_local;
                atmo_counter = kAtmoSkipSteps;
            }
            --atmo_counter;

            double dr = density_ratio;  // local copy for lambda capture
            double im = inv_mach;

            auto dragAccel = [&](const Vector3& v) -> Vector3 {
                Vector3 vr = v - wind_vec;
                double  sp = vr.fastMagnitude();
                Vector3 a = {0.0, kGravityFps2, 0.0};
                if (sp > 0.0) {
                    double m = sp * im;
                    double d = dr
                        * shot.drag.spline.evalHinted(m, spline_hint)
                        * drag_coeff;
                    a -= vr * (d * sp);
                }
                a += shot.coriolis.acceleration(v);
                return a;
            };

            // --- RK45 Dormand-Prince step ---
            Vector3 k1 = dragAccel(vel);
            Vector3 k2 = dragAccel(vel + k1 * (dt * a21));
            Vector3 k3 = dragAccel(vel + k1 * (dt * a31) + k2 * (dt * a32));
            Vector3 k4 = dragAccel(vel + k1 * (dt * a41) + k2 * (dt * a42) + k3 * (dt * a43));
            Vector3 k5 = dragAccel(vel + k1 * (dt * a51) + k2 * (dt * a52) + k3 * (dt * a53) + k4 * (dt * a54));
            Vector3 k6 = dragAccel(vel + k1 * (dt * a61) + k2 * (dt * a62) + k3 * (dt * a63) + k4 * (dt * a64) + k5 * (dt * a65));

            // 5th-order solution
            Vector3 vel_new = vel + (k1 * b1 + k3 * b3 + k4 * b4 + k5 * b5 + k6 * b6) * dt;

            // Error estimate (difference between 4th and 5th order)
            Vector3 k7 = dragAccel(vel_new);
            Vector3 err_vec = (k1 * e1 + k3 * e3 + k4 * e4 + k5 * e5 + k6 * e6 + k7 * e7) * dt;
            double err = err_vec.fastMagnitude();

            if (err > kRK45Tol && dt > kMinDt) {
                // Reject step, shrink dt
                double scale = kSafetyFactor * std::pow(kRK45Tol / err, 0.25);
                dt *= (scale > 0.2) ? scale : 0.2;
                if (dt < kMinDt) dt = kMinDt;
                continue;  // retry with smaller step
            }

            // Accept step — trapezoidal position update (must use vel before overwrite)
            pos += (vel + vel_new) * (dt * 0.5);
            vel = vel_new;

            // Adapt step size for next iteration
            if (err > 0.0) {
                double scale = kSafetyFactor * std::pow(kRK45Tol / err, 0.2);
                if (scale > 4.0) scale = 4.0;
                dt *= scale;
            } else {
                dt *= 2.0;
            }
            if (dt > kMaxDt) dt = kMaxDt;
            if (dt < kMinDt) dt = kMinDt;

            if (vel.magnitudeSquared() < kMinVelocitySq) break;
            if (pos.y < kMaxDrop) break;
        }

        return pos.y;
    }

    /// Integrate to target distance and return the full trajectory point.
    /// Uses adaptive RK45 Dormand-Prince with atmosphere amortization.
    TrajectoryPoint traceToPoint(const ShotConfig& shot,
                                 double target_dist_ft) const
    {
        double mv = shot.effectiveMuzzleVel();
        double barrel_elev = shot.look_angle_rad + shot.zero_elevation;
        double barrel_az   = 0.0;
        if (shot.cant_angle_rad != 0.0) {
            barrel_elev = shot.look_angle_rad
                        + std::cos(shot.cant_angle_rad) * shot.zero_elevation;
            barrel_az   = std::sin(shot.cant_angle_rad) * shot.zero_elevation;
        }

        Vector3 pos = {0.0, -shot.sight_height_ft, 0.0};
        Vector3 vel = {
            mv * std::cos(barrel_elev) * std::cos(barrel_az),
            mv * std::sin(barrel_elev),
            mv * std::cos(barrel_elev) * std::sin(barrel_az)
        };

        Vector3 wind_vec = shot.wind.vector();
        double  dt = kDefaultDt;
        double  time = 0.0;

        // Pre-compute for performance
        AtmoPrecomp atmo_pre(shot.atmo);
        double drag_coeff = (shot.drag.bc > 0.0)
            ? kDragConvFactor / shot.drag.bc : 0.0;
        size_t spline_hint = shot.drag.spline.n / 2;
        double sg = shot.stabilityCoefficient();

        // Atmosphere amortization state
        double density_ratio, mach_local, inv_mach;
        atmo_pre.atAltitude(shot.atmo.altitude_ft + pos.y,
                            density_ratio, mach_local);
        inv_mach = 1.0 / mach_local;
        int atmo_counter = 0;

        // Store previous step for interpolation
        Vector3 prev_pos = pos;
        Vector3 prev_vel = vel;
        double  prev_time = 0.0;

        while (pos.x < target_dist_ft) {
            prev_pos  = pos;
            prev_vel  = vel;
            prev_time = time;

            // Refresh atmosphere periodically
            if (atmo_counter <= 0) {
                atmo_pre.atAltitude(shot.atmo.altitude_ft + pos.y,
                                    density_ratio, mach_local);
                inv_mach = 1.0 / mach_local;
                atmo_counter = kAtmoSkipSteps;
            }
            --atmo_counter;

            double dr = density_ratio;
            double im = inv_mach;

            auto accel = [&](const Vector3& v) -> Vector3 {
                Vector3 v_rel = v - wind_vec;
                double  speed = v_rel.fastMagnitude();
                Vector3 a = {0.0, kGravityFps2, 0.0};
                if (speed > 0.0) {
                    double mach = speed * im;
                    double drag = dr
                        * shot.drag.spline.evalHinted(mach, spline_hint)
                        * drag_coeff;
                    a -= v_rel * (drag * speed);
                }
                a += shot.coriolis.acceleration(v);
                return a;
            };

            // --- RK45 Dormand-Prince step ---
            Vector3 k1 = accel(vel);
            Vector3 k2 = accel(vel + k1 * (dt * a21));
            Vector3 k3 = accel(vel + k1 * (dt * a31) + k2 * (dt * a32));
            Vector3 k4 = accel(vel + k1 * (dt * a41) + k2 * (dt * a42) + k3 * (dt * a43));
            Vector3 k5 = accel(vel + k1 * (dt * a51) + k2 * (dt * a52) + k3 * (dt * a53) + k4 * (dt * a54));
            Vector3 k6 = accel(vel + k1 * (dt * a61) + k2 * (dt * a62) + k3 * (dt * a63) + k4 * (dt * a64) + k5 * (dt * a65));

            // 5th-order velocity
            Vector3 vel_new = vel + (k1 * b1 + k3 * b3 + k4 * b4 + k5 * b5 + k6 * b6) * dt;

            // Error estimate
            Vector3 k7 = accel(vel_new);
            Vector3 err_vec = (k1 * e1 + k3 * e3 + k4 * e4 + k5 * e5 + k6 * e6 + k7 * e7) * dt;
            double err = err_vec.fastMagnitude();

            if (err > kRK45Tol && dt > kMinDt) {
                double scale = kSafetyFactor * std::pow(kRK45Tol / err, 0.25);
                dt *= (scale > 0.2) ? scale : 0.2;
                if (dt < kMinDt) dt = kMinDt;
                continue;
            }

            // Accept step — trapezoidal position update
            pos += (vel + vel_new) * (dt * 0.5);
            vel = vel_new;
            time += dt;

            // Adapt step size
            if (err > 0.0) {
                double scale = kSafetyFactor * std::pow(kRK45Tol / err, 0.2);
                if (scale > 4.0) scale = 4.0;
                dt *= scale;
            } else {
                dt *= 2.0;
            }
            if (dt > kMaxDt) dt = kMaxDt;
            if (dt < kMinDt) dt = kMinDt;

            if (vel.magnitudeSquared() < kMinVelocitySq) break;
            if (pos.y < kMaxDrop) break;
        }

        // Interpolate to exact distance
        double frac = 0.0;
        if (pos.x != prev_pos.x) {
            frac = (target_dist_ft - prev_pos.x) / (pos.x - prev_pos.x);
        }
        Vector3 ipos = prev_pos + (pos - prev_pos) * frac;
        Vector3 ivel = prev_vel + (vel - prev_vel) * frac;
        double  itime = prev_time + (time - prev_time) * frac;

        // Apply spin drift (using pre-computed stability coefficient)
        ipos.z += shot.spinDrift(itime, sg);

        TrajectoryPoint pt;
        pt.time_s       = itime;
        pt.distance_ft  = ipos.x;
        pt.height_ft    = ipos.y;
        pt.windage_ft   = ipos.z;
        pt.velocity_fps = ivel.magnitude();
        double density_r, mach_fps;
        shot.atmo.atAltitude(shot.atmo.altitude_ft + ipos.y,
                             density_r, mach_fps);
        pt.mach           = pt.velocity_fps / mach_fps;
        pt.density_ratio  = density_r;
        pt.energy_ftlb    = shot.drag.weight_gr * pt.velocity_fps
                          * pt.velocity_fps / 450400.0;
        pt.drop_angle_rad = std::atan2(ipos.y, ipos.x);
        pt.wind_angle_rad = std::atan2(ipos.z, ipos.x);

        return pt;
    }

    /// Record a trajectory point (used by trajectory()).
    void recordPoint(std::vector<TrajectoryPoint>& points,
                     const ShotConfig& shot,
                     const Vector3& pos, const Vector3& vel,
                     double time, const Vector3& wind_vec) const
    {
        TrajectoryPoint pt;
        pt.time_s       = time;
        pt.distance_ft  = pos.x;
        pt.height_ft    = pos.y;
        pt.windage_ft   = pos.z + shot.spinDrift(time);
        pt.velocity_fps = vel.magnitude();

        double density_r, mach_fps;
        shot.atmo.atAltitude(shot.atmo.altitude_ft + pos.y,
                             density_r, mach_fps);
        pt.mach           = pt.velocity_fps / mach_fps;
        pt.density_ratio  = density_r;
        pt.energy_ftlb    = shot.drag.weight_gr * pt.velocity_fps
                          * pt.velocity_fps / 450400.0;
        pt.drop_angle_rad = std::atan2(pos.y, pos.x);
        pt.wind_angle_rad = std::atan2(pos.z + shot.spinDrift(time), pos.x);

        points.push_back(pt);
    }

    /// Ridder's method for guaranteed convergence.
    bool ridderZero(ShotConfig& shot, double lo, double hi) const {
        double f_lo = evalZero(shot, lo);
        double f_hi = evalZero(shot, hi);

        for (int i = 0; i < kMaxZeroIter; ++i) {
            double mid = 0.5 * (lo + hi);
            double f_mid = evalZero(shot, mid);

            double s = std::sqrt(f_mid * f_mid - f_lo * f_hi);
            if (s == 0.0) {
                shot.zero_elevation = mid;
                return true;
            }

            double sign = (f_lo - f_hi < 0.0) ? -1.0 : 1.0;
            double x_new = mid + (mid - lo) * sign * f_mid / s;
            double f_new = evalZero(shot, x_new);

            if (std::abs(f_new) < kZeroAccFt) {
                shot.zero_elevation = x_new;
                return true;
            }

            // Narrow bracket
            if (f_mid * f_new < 0.0) {
                if (f_mid < f_new) { lo = x_new; f_lo = f_new; hi = mid; f_hi = f_mid; }
                else               { lo = mid; f_lo = f_mid; hi = x_new; f_hi = f_new; }
            } else if (f_lo * f_new < 0.0) {
                hi = x_new; f_hi = f_new;
            } else {
                lo = x_new; f_lo = f_new;
            }

            if (std::abs(hi - lo) < 1e-12) break;
        }

        shot.zero_elevation = 0.5 * (lo + hi);
        return true;
    }

    /// Evaluate zero-finding error for a given elevation.
    double evalZero(ShotConfig& shot, double elev) const {
        shot.zero_elevation = elev;
        return traceToDistance(shot, shot.zero_distance_ft);
    }
};

}  // namespace ballistic
