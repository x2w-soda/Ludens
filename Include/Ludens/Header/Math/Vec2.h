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

/// @brief compare each component in vector, using epsilon tolerance for floating point precision
template <typename T>
inline bool operator==(const TVec2<T>& lhs, const TVec2<T>& rhs)
{
    if constexpr (std::is_same_v<T, float>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F32 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F32;
    } else if constexpr (std::is_same_v<T, double>)
    {
        return std::abs(lhs.x - rhs.x) < LD_EPSILON_F64 &&
               std::abs(lhs.y - rhs.y) < LD_EPSILON_F64;
    } else
        return lhs.x == rhs.x && lhs.y == rhs.y;
}

using Vec2 = TVec2<float>;
using IVec2 = TVec2<int>;

} // namespace LD