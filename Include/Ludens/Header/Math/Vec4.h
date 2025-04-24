#pragma once

#include <Ludens/Header/Math/Vec3.h>

namespace LD {

// clang-format off
template <typename T>
struct TVec4
{
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
// clang-format on

using Vec4 = TVec4<float>;
using IVec4 = TVec4<int>;

} // namespace LD