#pragma once

#include <Ludens/Header/Types.h>

namespace LD {

// clang-format off
template <typename T>
struct TQuat
{
    T x, y, z, w;
    
    TQuat() = default;
    TQuat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    /// @brief create from array of 4 scalar elements, the last element being the real part
	template<typename TElement>
	static TQuat from_data(const TElement* a)
	{
		T x = static_cast<T>(a[0]);
		T y = static_cast<T>(a[1]);
		T z = static_cast<T>(a[2]);
		T w = static_cast<T>(a[3]);
		return { x, y, z, w };
	}
};
// clang-format on

using Quat = TQuat<float>;

static_assert(LD::IsTrivial<Quat>);

} // namespace LD