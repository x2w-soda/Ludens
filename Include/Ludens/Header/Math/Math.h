#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <type_traits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.785398163397448309616
#endif

#define LD_PI M_PI
#define LD_PI_2 M_PI_2
#define LD_PI_4 M_PI_4

#define LD_EPSILON_F32 ((float)1e-6)
#define LD_EPSILON_F64 (1e-12)

#define LD_TO_RADIANS(DEG) (DEG * (LD_PI / 180))
#define LD_TO_DEGREES(RAD) (RAD * (180 / LD_PI))

#define LD_SIN(X) std::sin(X)
#define LD_COS(X) std::cos(X)
#define LD_TAN(X) std::tan(X)

#define LD_ASIN(X) std::asin(X)
#define LD_ACOS(X) std::acos(X)
#define LD_ATAN(X) std::atan(X)
#define LD_ATAN2(X, Y) std::atan2(X, Y)

#define LD_ABS(X) std::abs(X)
#define LD_SQRT(X) std::sqrt(X)

/// @brief check if a value is zero with epsilon tolerance for floating points
template <typename T>
inline bool is_zero_epsilon(T value)
{
    if constexpr (std::is_same_v<T, float>)
        return LD_ABS(value) < LD_EPSILON_F32;
    else if constexpr (std::is_same_v<T, double>)
        return LD_ABS(value) < LD_EPSILON_F64;
    else
        return value == (T)0;
}

/// @brief chech if two values are equal with epsilon tolerance for floating points
template <typename T>
inline bool is_equal_epsilon(T lhs, T rhs)
{
    return is_zero_epsilon<T>(static_cast<T>(lhs - rhs));
}