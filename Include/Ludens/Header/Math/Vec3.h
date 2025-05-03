#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec2.h>

namespace LD {

template <typename T>
struct TVec3
{
    // clang-format off
	union { T x; T r; };
	union { T y; T g; };
	union { T z; T b; };

	TVec3() : x((T)0), y((T)0), z((T)0) {}
	TVec3(T v) : x((T)v), y((T)v), z((T)v) {}
	TVec3(T x, T y, T z) : x((T)x), y((T)y), z((T)z) {}
    TVec3(T x, const TVec2<T>& v) : x((T)x), y(v.x), z(v.y) {}
    TVec3(const TVec2<T>& v, T z) : x(v.x), y(v.y), z((T)z) {}
	TVec3(const TVec3& other) : x(other.x), y(other.y), z(other.z) {}
    // clang-format on

    inline T length_squared() const { return x * x + y * y + z * z; }

    /// @brief create from array of 3 scalar elements
    template <typename TElement>
    static TVec3 from_data(const TElement* a)
    {
        T x = static_cast<T>(a[0]);
        T y = static_cast<T>(a[1]);
        T z = static_cast<T>(a[2]);
        return {x, y, z};
    }

    /// @brief dot product between two vectors
    static T dot(const TVec3& lhs, const TVec3& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    /// @brief cross product between two vectors
    static TVec3 cross(const TVec3& lhs, const TVec3& rhs)
    {
        return {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x};
    }

    /// @brief normalize a vector
    static TVec3 normalize(const TVec3& v)
    {
        return v / LD_SQRT(v.x * v.x + v.y * v.y + v.z * v.z);
    }
};

/// @brief compare each component in vector, using epsilon tolerance for floating point precision
template <typename T>
inline bool operator==(const TVec3<T>& lhs, const TVec3<T>& rhs)
{
    if constexpr (std::is_same_v<T, float>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F32 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F32 &&
               std::abs(lhs.z - rhs.z) < LD_EPSILON_F32;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F64 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F64 &&
               std::abs(lhs.z - rhs.z) < LD_EPSILON_F64;
    }
    else
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

#define LD_VEC3_SCALAR(OP)                              \
    template <typename T>                               \
    inline TVec3<T> operator OP(const TVec3<T>& v, T s) \
    {                                                   \
        return TVec3<T>(v.x OP s, v.y OP s, v.z OP s);  \
    }

#define LD_VEC3_ARITH(OP, OP_ASSIGN)                                        \
    template <typename T>                                                   \
    inline TVec3<T> operator OP(const TVec3<T>& lhs, const TVec3<T>& rhs)   \
    {                                                                       \
        return TVec3<T>(lhs.x OP rhs.x, lhs.y OP rhs.y, lhs.z OP rhs.z);    \
    }                                                                       \
    template <typename T>                                                   \
    inline TVec3<T>& operator OP_ASSIGN(TVec3<T>& lhs, const TVec3<T>& rhs) \
    {                                                                       \
        lhs = lhs OP rhs;                                                   \
        return lhs;                                                         \
    }

LD_VEC3_SCALAR(+);
LD_VEC3_SCALAR(-);
LD_VEC3_SCALAR(*);
LD_VEC3_SCALAR(/);
LD_VEC3_ARITH(+, +=);
LD_VEC3_ARITH(-, -=);
LD_VEC3_ARITH(*, *=);
LD_VEC3_ARITH(/, /=);

#undef LD_VEC3_ARITH
#undef LD_VEC3_SCALAR

using Vec3 = TVec3<float>;
using IVec3 = TVec3<int>;

} // namespace LD