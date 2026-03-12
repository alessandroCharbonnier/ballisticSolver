#pragma once
/// @file drag_model.h
/// @brief Drag model combining BC, drag table, and projectile properties.

#include <cmath>
#include "constants.h"
#include "drag_tables.h"
#include "interpolation.h"

namespace ballistic {

/// A ballistic coefficient point (for multi-BC profiles).
struct BCPoint {
    double bc;
    double velocity_fps;  // velocity at which this BC was measured
};

/// Complete drag model for a projectile.
struct DragModel {
    double           bc            = 0.0;
    DragTableId      table_id      = DragTableId::G7;
    double           weight_gr     = 0.0;    // grains
    double           diameter_in   = 0.0;    // inches
    double           length_in     = 0.0;    // inches (for stability calc)

    // Derived
    double           form_factor   = 0.0;
    double           sect_density  = 0.0;

    // Pre-computed spline
    PchipSpline      spline;

    DragModel() = default;

    /// Construct with single BC.
    DragModel(double bc_, DragTableId table, double weight = 0.0,
              double diameter = 0.0, double length = 0.0)
        : bc(bc_), table_id(table), weight_gr(weight),
          diameter_in(diameter), length_in(length)
    {
        computeDerived();
        buildSpline();
    }

    /// Construct with multi-BC (interpolates form factors, produces custom curve).
    DragModel(const BCPoint* bc_points, size_t num_bc_points,
              DragTableId table, double weight, double diameter, double length,
              double mach_at_sea_level_fps)
        : table_id(table), weight_gr(weight),
          diameter_in(diameter), length_in(length)
    {
        if (num_bc_points == 0) return;
        bc = bc_points[0].bc;  // default to first
        computeDerived();
        buildMultiBCSpline(bc_points, num_bc_points, mach_at_sea_level_fps);
    }

    /// Lookup Cd at a given Mach number (from pre-computed spline).
    double cdAtMach(double mach) const {
        return spline.eval(mach);
    }

    /// Compute drag deceleration factor for a given Mach number.
    /// drag_factor = density_ratio * Cd(mach) * kDragConvFactor / BC
    double dragByMach(double mach) const {
        if (bc <= 0.0) return 0.0;
        return spline.eval(mach) * kDragConvFactor / bc;
    }

private:
    void computeDerived() {
        if (diameter_in > 0.0 && weight_gr > 0.0) {
            sect_density = weight_gr / (diameter_in * diameter_in * kGrainsPerPound);
            if (bc > 0.0) form_factor = sect_density / bc;
        }
    }

    void buildSpline() {
        size_t size = 0;
        const DragPoint* table = getDragTable(table_id, size);
        if (table && size > 0) {
            spline.build(table, size);
        }
    }

    /// Build a custom spline from multi-BC points by modifying the standard table.
    void buildMultiBCSpline(const BCPoint* pts, size_t num_pts,
                            double mach_sea_level_fps)
    {
        size_t table_size = 0;
        const DragPoint* table = getDragTable(table_id, table_size);
        if (!table || table_size == 0 || num_pts == 0) return;

        // Convert BC points to (Mach, form_factor) pairs
        std::vector<double> pt_mach(num_pts);
        std::vector<double> pt_ff(num_pts);

        for (size_t i = 0; i < num_pts; ++i) {
            pt_mach[i] = pts[i].velocity_fps / mach_sea_level_fps;
            double sd = weight_gr / (diameter_in * diameter_in * kGrainsPerPound);
            pt_ff[i] = sd / pts[i].bc;
        }

        // Build modified drag table with interpolated form factors
        std::vector<DragPoint> modified(table_size);
        for (size_t i = 0; i < table_size; ++i) {
            double m = table[i].mach;
            double ff;

            // Find bounding BC points for this Mach
            if (m <= pt_mach[0]) {
                ff = pt_ff[0];
            } else if (m >= pt_mach[num_pts - 1]) {
                ff = pt_ff[num_pts - 1];
            } else {
                // Linear interpolation between BC points
                size_t j = 0;
                while (j < num_pts - 1 && pt_mach[j + 1] < m) ++j;
                double t = (m - pt_mach[j]) / (pt_mach[j + 1] - pt_mach[j]);
                ff = pt_ff[j] + t * (pt_ff[j + 1] - pt_ff[j]);
            }

            modified[i].mach = m;
            modified[i].cd = table[i].cd * ff / form_factor;
        }

        spline.build(modified.data(), modified.size());

        // Use average BC for the factor
        double avg_bc = 0.0;
        for (size_t i = 0; i < num_pts; ++i) avg_bc += pts[i].bc;
        bc = avg_bc / num_pts;
        computeDerived();
    }
};

}  // namespace ballistic
