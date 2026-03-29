#pragma once

// -----------------------------------------------------------------------
// Vec3 -- minimal 3D vector for trace and movement operations
// -----------------------------------------------------------------------
struct Vec3
{
    float x, y, z;

    Vec3() : x(0.f), y(0.f), z(0.f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};
