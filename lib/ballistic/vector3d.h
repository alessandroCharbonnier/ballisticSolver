#pragma once
/// @file vector3d.h
/// @brief Immutable 3-component vector for ballistic calculations.
///
/// Coordinate convention (right-hand, shooter-centric):
///   x = down-range  (positive away from shooter)
///   y = vertical     (positive up)
///   z = lateral      (positive to shooter's right)

#include <cmath>

namespace ballistic {

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr Vector3() = default;
    constexpr Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    // --- Arithmetic ---
    constexpr Vector3 operator+(const Vector3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    constexpr Vector3 operator-(const Vector3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    constexpr Vector3 operator*(double s)         const { return {x * s,   y * s,   z * s  }; }
    constexpr Vector3 operator/(double s)         const { return {x / s,   y / s,   z / s  }; }
    constexpr Vector3 operator-()                 const { return {-x, -y, -z}; }

    Vector3& operator+=(const Vector3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vector3& operator-=(const Vector3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
    Vector3& operator*=(double s)         { x *= s;   y *= s;   z *= s;   return *this; }

    /// Dot product.
    constexpr double dot(const Vector3& b) const { return x * b.x + y * b.y + z * b.z; }

    /// Euclidean magnitude.
    double magnitude() const { return std::sqrt(x * x + y * y + z * z); }

    /// Euclidean magnitude using hardware float sqrt + Newton refinement.
    /// ~14 digits of precision — sufficient for ballistic integration.
    double fastMagnitude() const {
        double sq = x * x + y * y + z * z;
        if (sq <= 0.0) return 0.0;
        float approx = sqrtf(static_cast<float>(sq));
        double d = static_cast<double>(approx);
        return 0.5 * (d + sq / d);  // one Newton-Raphson step
    }

    /// Squared magnitude (avoids sqrt for comparisons).
    constexpr double magnitudeSquared() const { return x * x + y * y + z * z; }

    /// Unit vector (returns zero vector if magnitude < eps).
    Vector3 normalized(double eps = 1e-10) const {
        double m = magnitude();
        if (m < eps) return {0.0, 0.0, 0.0};
        return *this / m;
    }
};

/// Scalar * vector.
constexpr Vector3 operator*(double s, const Vector3& v) { return v * s; }

/// Sentinel zero vector.
constexpr Vector3 kZeroVector{0.0, 0.0, 0.0};

}  // namespace ballistic
