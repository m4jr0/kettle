// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include "format.h"

#include <cmath>
#include <type_traits>

namespace kettle
{
template<typename T>
    requires std::is_floating_point_v<T>
inline constexpr T kEpsilon = T(1e-5);

template<>
inline constexpr float kEpsilon<float> = 1e-5f;

template<>
inline constexpr double kEpsilon<double> = 1e-9;

template<typename T>
    requires std::is_floating_point_v<T>
inline bool almostEqual(T a, T b, T eps = kEpsilon<T>)
{
    return std::fabs(a - b) <= eps;
}

template<typename T>
    requires std::is_floating_point_v<T>
inline bool lessOrAlmostEqual(T a, T b, T eps = kEpsilon<T>)
{
    return a < b || almostEqual(a, b, eps);
}

template<typename T>
    requires std::is_floating_point_v<T>
inline bool greaterOrAlmostEqual(T a, T b, T eps = kEpsilon<T>)
{
    return a > b || almostEqual(a, b, eps);
}

inline bool equal(const Vec2& a, const Vec2& b)
{
    return a.x == b.x && a.y == b.y;
}

inline bool almostEqual(const Vec2& a, const Vec2& b, float eps = kEpsilon<float>)
{
    return almostEqual(a.x, b.x, eps) && almostEqual(a.y, b.y, eps);
}

inline bool equal(const Vec3& a, const Vec3& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline bool almostEqual(const Vec3& a, const Vec3& b, float eps = kEpsilon<float>)
{
    return almostEqual(a.x, b.x, eps) && almostEqual(a.y, b.y, eps) && almostEqual(a.z, b.z, eps);
}


inline bool notNearlyEqual(const Vec3& a, const Vec3& b, float eps = kEpsilon<float>)
{
    return !almostEqual(a, b, eps);
}

inline bool equal(const Vec4& a, const Vec4& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

inline bool almostEqual(const Vec4& a, const Vec4& b, float eps = kEpsilon<float>)
{
    return almostEqual(a.x, b.x, eps) && almostEqual(a.y, b.y, eps) && almostEqual(a.z, b.z, eps) && almostEqual(a.w, b.w, eps);
}


inline bool notNearlyEqual(const Vec4 a, const Vec4 b, float eps = kEpsilon<float>)
{
    return !almostEqual(a, b, eps);
}
} // namespace kettle