/*
 * vector3d.h - 3D Vector Mathematics
 *
 * Immutable 3D vector for ballistic trajectory calculations.
 * Components: x (downrange), y (vertical/up), z (lateral/right)
 *
 * Port of py_ballisticcalc.vector.Vector
 */

#pragma once

#include <cmath>

namespace bc {

struct Vector3D {
    double x;  // downrange
    double y;  // vertical (positive = up)
    double z;  // lateral (positive = right)

    constexpr Vector3D() : x(0.0), y(0.0), z(0.0) {}
    constexpr Vector3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    // Magnitude (Euclidean norm)
    double magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    // Normalize to unit vector
    Vector3D normalize() const {
        double mag = magnitude();
        if (mag < 1e-15) return {0.0, 0.0, 0.0};
        return {x / mag, y / mag, z / mag};
    }

    // Dot product
    double dot(const Vector3D& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // Multiply by scalar
    Vector3D operator*(double s) const {
        return {x * s, y * s, z * s};
    }

    // Divide by scalar
    Vector3D operator/(double s) const {
        return {x / s, y / s, z / s};
    }

    // Vector addition
    Vector3D operator+(const Vector3D& o) const {
        return {x + o.x, y + o.y, z + o.z};
    }

    // Vector subtraction
    Vector3D operator-(const Vector3D& o) const {
        return {x - o.x, y - o.y, z - o.z};
    }

    // In-place addition
    Vector3D& operator+=(const Vector3D& o) {
        x += o.x; y += o.y; z += o.z;
        return *this;
    }

    // In-place subtraction
    Vector3D& operator-=(const Vector3D& o) {
        x -= o.x; y -= o.y; z -= o.z;
        return *this;
    }

    // Negate
    Vector3D operator-() const {
        return {-x, -y, -z};
    }

    // Comparison
    bool operator==(const Vector3D& o) const {
        return x == o.x && y == o.y && z == o.z;
    }

    bool operator!=(const Vector3D& o) const {
        return !(*this == o);
    }
};

// Scalar * Vector
inline Vector3D operator*(double s, const Vector3D& v) {
    return {s * v.x, s * v.y, s * v.z};
}

// Zero vector constant
constexpr Vector3D ZERO_VECTOR{0.0, 0.0, 0.0};

}  // namespace bc
