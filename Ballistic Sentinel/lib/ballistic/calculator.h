#pragma once
/// @file calculator.h
/// @brief High-level ballistic calculator API.
///
/// Usage:
///   ballistic::Calculator calc;
///   calc.configure(bc, table, weight, diameter, length,
///                  mv_fps, sight_height_in, twist_in, zero_range_yd);
///   calc.setAtmosphere(temp_f, pressure_inhg, humidity, altitude_ft);
///   calc.setWind(speed_fps, direction_deg);
///   calc.setCoriolis(latitude_deg, azimuth_deg);
///   calc.solve();   // find zero
///   auto result = calc.correction(distance_yd, unit);

#include "engine_rk4.h"

namespace ballistic {

class Calculator {
public:
    Calculator() = default;

    // -----------------------------------------------------------------------
    //  Configuration
    // -----------------------------------------------------------------------

    /// Configure projectile and rifle.
    void configure(double bc, DragTableId table,
                   double weight_gr, double diameter_in, double length_in,
                   double muzzle_vel_fps,
                   double sight_height_in, double twist_in,
                   double zero_range_yd)
    {
        shot_.drag = DragModel(bc, table, weight_gr, diameter_in, length_in);
        shot_.muzzle_vel_fps  = muzzle_vel_fps;
        shot_.sight_height_ft = sight_height_in / kInchesPerFoot;
        shot_.twist_in        = twist_in;
        shot_.zero_distance_ft = zero_range_yd * kFeetPerYard;
        dirty_ = true;
    }

    /// Configure with multi-BC.
    void configureMultiBC(const BCPoint* bc_pts, size_t count,
                          DragTableId table,
                          double weight_gr, double diameter_in, double length_in,
                          double muzzle_vel_fps,
                          double sight_height_in, double twist_in,
                          double zero_range_yd)
    {
        double mach_fps = shot_.atmo.mach_fps;
        if (mach_fps <= 0.0) mach_fps = 1116.0;  // sea level default

        shot_.drag = DragModel(bc_pts, count, table, weight_gr,
                               diameter_in, length_in, mach_fps);
        shot_.muzzle_vel_fps  = muzzle_vel_fps;
        shot_.sight_height_ft = sight_height_in / kInchesPerFoot;
        shot_.twist_in        = twist_in;
        shot_.zero_distance_ft = zero_range_yd * kFeetPerYard;
        dirty_ = true;
    }

    /// Update atmospheric conditions (call before correction()).
    void setAtmosphere(double temp_f, double pressure_inhg,
                       double humidity, double altitude_ft = 0.0)
    {
        shot_.atmo = Atmosphere(temp_f, pressure_inhg, humidity, altitude_ft);
        dirty_ = true;
    }

    /// Set wind conditions.
    /// @param speed_fps   Wind speed in fps.
    /// @param dir_deg     Wind direction in degrees (0 = tail, 90 = from left).
    void setWind(double speed_fps, double dir_deg) {
        shot_.wind.velocity_fps  = speed_fps;
        shot_.wind.direction_rad = dir_deg * kDegToRad;
    }

    /// Set wind from speed (mph) and clock direction (12 = tail, 3 = from left).
    void setWindClock(double speed_mph, double clock) {
        shot_.wind.velocity_fps  = speed_mph * 5280.0 / 3600.0;
        shot_.wind.direction_rad = (clock - 12.0) * 30.0 * kDegToRad;
    }

    /// Enable Coriolis correction.
    void setCoriolis(double latitude_deg, double azimuth_deg) {
        shot_.coriolis.enabled  = true;
        shot_.coriolis.latitude = latitude_deg * kDegToRad;
        shot_.coriolis.azimuth  = azimuth_deg * kDegToRad;
        shot_.coriolis.precompute();
    }

    void disableCoriolis() {
        shot_.coriolis.enabled = false;
    }

    /// Set look angle (incline) in degrees.
    void setLookAngle(double degrees) {
        shot_.look_angle_rad = degrees * kDegToRad;
    }

    /// Configure powder temperature sensitivity.
    void setPowderSensitivity(double base_temp_f, double modifier) {
        shot_.powder_temp_f  = base_temp_f;
        shot_.temp_modifier  = modifier;
        shot_.use_temp_sens  = (modifier != 0.0);
        dirty_ = true;
    }

    /// Calculate powder sensitivity from two velocity/temperature pairs.
    /// v1, t1 = reference;  v2, t2 = cold/hot test.
    void calcPowderSensitivity(double v1_fps, double t1_f,
                               double v2_fps, double t2_f)
    {
        double dv = std::abs(v1_fps - v2_fps);
        double dt = std::abs(t1_f - t2_f);
        if (dt <= 0.0) return;
        double v_min = std::min(v1_fps, v2_fps);
        shot_.temp_modifier  = (dv / dt) * (15.0 / v_min);
        shot_.powder_temp_f  = t1_f;
        shot_.use_temp_sens  = true;
        dirty_ = true;
    }

    // -----------------------------------------------------------------------
    //  Solve (find zero)
    // -----------------------------------------------------------------------

    /// Compute barrel elevation for zeroing.  Must be called after configure()
    /// and setAtmosphere(), before correction().
    /// @return true on convergence.
    bool solve() {
        bool ok = engine_.findZero(shot_);
        dirty_ = false;
        return ok;
    }

    // -----------------------------------------------------------------------
    //  Query
    // -----------------------------------------------------------------------

    /// Get correction at a given distance (yards).
    CorrectionResult correction(double distance_yd,
                                CorrectionUnit unit = CorrectionUnit::MOA,
                                double click_size_rad = 0.0) const
    {
        double dist_ft = distance_yd * kFeetPerYard;
        return engine_.computeCorrection(shot_, dist_ft, unit, click_size_rad);
    }

    /// Get correction at a given distance in metres.
    CorrectionResult correctionMetric(double distance_m,
                                      CorrectionUnit unit = CorrectionUnit::MOA,
                                      double click_size_rad = 0.0) const
    {
        double dist_ft = distance_m * kFeetPerMetre;
        return engine_.computeCorrection(shot_, dist_ft, unit, click_size_rad);
    }

    /// Full trajectory (points every step_yd).
    std::vector<TrajectoryPoint> trajectory(double max_yd,
                                            double step_yd = 100.0) const
    {
        return engine_.trajectory(shot_, max_yd * kFeetPerYard,
                                  step_yd * kFeetPerYard);
    }

    /// Access current shot config (read-only).
    const ShotConfig& shotConfig() const { return shot_; }

    /// Whether solve() needs to be re-called.
    bool isDirty() const { return dirty_; }

private:
    ShotConfig shot_;
    EngineRK4  engine_;
    bool       dirty_ = true;
};

}  // namespace ballistic
