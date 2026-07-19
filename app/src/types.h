#pragma once

#include <cmath>

struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    vec2() = default;
    vec2(float x, float y) : x(x), y(y) {}

    vec2 operator+(const vec2& other) const { return {x + other.x, y + other.y}; }
    vec2 operator-(const vec2& other) const { return {x - other.x, y - other.y}; }
    vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

    vec2& operator+=(const vec2& other) { x += other.x; y += other.y; return *this; }
    vec2& operator-=(const vec2& other) { x -= other.x; y -= other.y; return *this; }
    vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }

    bool operator==(const vec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const vec2& other) const { return !(*this == other); }

    float length() const { return std::sqrt(x * x + y * y); }
    float length_squared() const { return x * x + y * y; }

    vec2 normalized() const {
        float len = length();
        if (len > 0.0f) return {x / len, y / len};
        return {0.0f, 0.0f};
    }

    float dot(const vec2& other) const { return x * other.x + y * other.y; }
    float cross(const vec2& other) const { return x * other.y - y * other.x; }
};

inline vec2 operator*(float scalar, const vec2& v) { return v * scalar; }

// Rotates 90 degrees counter-clockwise in a y-up frame -- clockwise on screen, where y points down.
inline vec2 perp(const vec2& v) { return {-v.y, v.x}; }
