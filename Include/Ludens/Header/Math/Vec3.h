#pragma once

namespace LD {

// clang-format off
template <typename T>
struct TVec3
{
	union { T x; T r; };
	union { T y; T g; };
	union { T z; T b; };

	TVec3() : x((T)0), y((T)0), z((T)0) {}
	TVec3(T v) : x((T)v), y((T)v), z((T)v) {}
	TVec3(T x, T y, T z) : x((T)x), y((T)y), z((T)z) {}
	TVec3(const TVec3& other) : x(other.x), y(other.y), z(other.z) {}

	inline T length_squared() const { return x*x + y*y + z*z; }

	/// @brief create from array of 3 scalar elements
	template<typename TElement>
	static TVec3 from_data(const TElement* a)
	{
		T x = static_cast<T>(a[0]);
		T y = static_cast<T>(a[1]);
		T z = static_cast<T>(a[2]);
		return { x, y, z };
	}

	/// @brief dot product between two vectors
	static T dot(const TVec3& lhs, const TVec3& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}
};
// clang-format on

using Vec3 = TVec3<float>;
using IVec3 = TVec3<int>;

} // namespace LD