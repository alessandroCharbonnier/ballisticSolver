#pragma once
/// @file interpolation.h
/// @brief PCHIP (Piecewise Cubic Hermite Interpolating Polynomial) spline.
///
/// Monotone-preserving interpolation using Fritsch-Carlson method.
/// Used for drag coefficient lookup by Mach number.

#include <cmath>
#include <cstddef>
#include <vector>

#include "drag_tables.h"

namespace ballistic {

/// Pre-computed PCHIP spline for fast evaluation.
struct PchipSpline {
    std::vector<double> x;   // knot positions (Mach numbers)
    std::vector<double> a;   // y values
    std::vector<double> b;   // first derivative (slope)
    std::vector<double> c;   // second-order coefficient
    std::vector<double> d;   // third-order coefficient
    size_t n = 0;            // number of knots

    PchipSpline() = default;

    /// Build spline from drag table array.
    void build(const DragPoint* table, size_t size) {
        n = size;
        if (n < 2) return;

        x.resize(n);
        a.resize(n);
        b.resize(n);
        c.resize(n);
        d.resize(n);

        // Copy data
        for (size_t i = 0; i < n; ++i) {
            x[i] = table[i].mach;
            a[i] = table[i].cd;
        }

        // Compute intervals and slopes
        std::vector<double> h(n - 1);
        std::vector<double> delta(n - 1);
        for (size_t i = 0; i < n - 1; ++i) {
            h[i] = x[i + 1] - x[i];
            delta[i] = (a[i + 1] - a[i]) / h[i];
        }

        // Fritsch-Carlson monotone slopes
        computeSlopes(delta, h);

        // Compute cubic coefficients for each interval
        for (size_t i = 0; i < n - 1; ++i) {
            c[i] = (3.0 * delta[i] - 2.0 * b[i] - b[i + 1]) / h[i];
            d[i] = (b[i] + b[i + 1] - 2.0 * delta[i]) / (h[i] * h[i]);
        }
        c[n - 1] = 0.0;
        d[n - 1] = 0.0;
    }

    /// Evaluate Cd at a given Mach number.
    double eval(double mach) const {
        if (n < 2) return 0.0;

        // Clamp to table range
        if (mach <= x[0])     return a[0];
        if (mach >= x[n - 1]) return a[n - 1];

        // Binary search for interval
        size_t lo = 0, hi = n - 1;
        while (hi - lo > 1) {
            size_t mid = (lo + hi) / 2;
            if (x[mid] <= mach) lo = mid;
            else                hi = mid;
        }

        // Horner's rule:  y = a + dx*(b + dx*(c + dx*d))
        double dx = mach - x[lo];
        return a[lo] + dx * (b[lo] + dx * (c[lo] + dx * d[lo]));
    }

private:
    /// Fritsch-Carlson method for monotone-preserving slopes.
    void computeSlopes(const std::vector<double>& delta,
                       const std::vector<double>& h) {
        // End-point slopes
        b[0] = delta[0];
        b[n - 1] = delta[n - 2];

        for (size_t i = 1; i < n - 1; ++i) {
            if (delta[i - 1] * delta[i] <= 0.0) {
                // Sign change or zero — set slope to zero (monotone preservation)
                b[i] = 0.0;
            } else {
                // Harmonic mean of adjacent secants (weighted by interval widths)
                double w1 = 2.0 * h[i] + h[i - 1];
                double w2 = h[i] + 2.0 * h[i - 1];
                b[i] = (w1 + w2) / (w1 / delta[i - 1] + w2 / delta[i]);
            }
        }
    }
};

/// Simple linear interpolation between two points.
inline double lerp(double x, double x0, double y0, double x1, double y1) {
    if (x1 == x0) return y0;
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

}  // namespace ballistic
