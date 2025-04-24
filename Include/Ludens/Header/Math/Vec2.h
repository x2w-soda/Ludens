#pragma once

namespace LD {

// clang-format off
template <typename T>
struct TVec2
{
	union { T x; T r; };
	union { T y; T g; };

	TVec2() : x((T)0), y((T)0) {}
	TVec2(T v) : x((T)v), y((T)v) {}
	TVec2(T x, T y) : x((T)x), y((T)y) {}
	TVec2(const TVec2& other) : x(other.x), y(other.y) {}

	inline T length_squared() const { return x*x + y*y; }

	/// @brief create from array of 2 scalar elements
	template<typename TElement>
	static TVec2 from_data(const TElement* a)
	{
		T x = static_cast<T>(a[0]);
		T y = static_cast<T>(a[1]);
		return { x, y };
	}

	/// @brief dot product between two vectors
	static T dot(const TVec2& lhs, const TVec2& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}
};
// clang-format on

using Vec2 = TVec2<float>;
using IVec2 = TVec2<int>;

} // namespace LD