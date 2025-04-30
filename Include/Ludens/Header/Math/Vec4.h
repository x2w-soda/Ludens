#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

template <typename T>
struct TVec4
{
    // clang-format off
	union { T x; T r; };
	union { T y; T g; };
	union { T z; T b; };
	union { T w; T a; };

    TVec4() : x((T)0), y((T)0), z((T)0), w((T)0) {}
    TVec4(T v) : x((T)v), y((T)v), z((T)v), w((T)v) {}
    TVec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
	TVec4(const TVec4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
    TVec4(const TVec2<T>& v1, const TVec2<T>& v2) : x(v1.x), y(v1.y), z(v2.x), w(v2.y) {}
	TVec4(const TVec3<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}
	TVec4(T x, const TVec3<T>& v) : x(x), y(v.x), z(v.y), w(v.z) {}
    // clang-format on

	inline T length_squared() const { return x*x + y*y + z*z + w*w; }

	/// @brief create from array of 4 scalar elements
    template <typename TElement>
    static TVec4 from_data(const TElement* a)
    {
        T x = static_cast<T>(a[0]);
        T y = static_cast<T>(a[1]);
        T z = static_cast<T>(a[2]);
        T w = static_cast<T>(a[3]);
        return {x, y, z, w};
    }

    /// @brief dot product between two vectors
	static T dot(const TVec4& lhs, const TVec4& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
	}
};

/// @brief compare each component in vector, using epsilon tolerance for floating point precision
template <typename T>
inline bool operator==(const TVec4<T>& lhs, const TVec4<T>& rhs)
{
    if constexpr (std::is_same_v<T, float>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F32 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F32 &&
               std::abs(lhs.z - rhs.z) < LD_EPSILON_F32 &&
               std::abs(lhs.w - rhs.w) < LD_EPSILON_F32;
    } else if constexpr (std::is_same_v<T, double>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F64 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F64 &&
               std::abs(lhs.z - rhs.z) < LD_EPSILON_F64 &&
               std::abs(lhs.w - rhs.w) < LD_EPSILON_F64;
    } else
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

#define LD_VEC4_SCALAR(OP)                                       \
    template <typename T>                                        \
    inline TVec4<T> operator OP(const TVec4<T>& v, T s)          \
    {                                                            \
        return TVec4<T>(v.x OP s, v.y OP s, v.z OP s, v.w OP s); \
    }

#define LD_VEC4_ARITH(OP)                                                                \
    template <typename T>                                                                \
    inline TVec4<T> operator OP(const TVec4<T>& lhs, const TVec4<T>& rhs)                \
    {                                                                                    \
        return TVec4<T>(lhs.x OP rhs.x, lhs.y OP rhs.y, lhs.z OP rhs.z, lhs.w OP rhs.w); \
    }

LD_VEC4_SCALAR(+);
LD_VEC4_SCALAR(-);
LD_VEC4_SCALAR(*);
LD_VEC4_SCALAR(/);
LD_VEC4_ARITH(+);
LD_VEC4_ARITH(-);
LD_VEC4_ARITH(*);
LD_VEC4_ARITH(/);

#undef LD_VEC4_ARITH
#undef LD_VEC4_SCALAR

using Vec4 = TVec4<float>;
using IVec4 = TVec4<int>;

} // namespace LD