#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Vec2.h"

namespace LD {

	template <typename T>
	struct TRect2D;


	using Rect2D = TRect2D<float>;
	using DRect2D = TRect2D<double>;
	using IRect2D = TRect2D<i32>;

	// Axis Aligned Bounding Box
	template <typename T>
	struct TRect2D
	{
		T x, y;
		T w, h;

		TRect2D() = default;
		TRect2D(T x, T y, T w, T h)
			: x(x), y(y), w(w), h(h) {}
		TRect2D(const TVec2<T>& min, const TVec2<T>& max)
			: x(min.x), y(min.y), w(max.x - min.x), h(max.y - min.y) {}
		TRect2D(const TVec2<T>& pos, T w, T h)
			: x(pos.x), y(pos.y), w(w), h(h) {}

		inline TVec2<T> Min() const { return { x, y }; }
		inline TVec2<T> Max() const { return { x + w, y + h }; }
		inline TVec2<T> Center() const { return { x + w / static_cast<T>(2), y + h / static_cast<T>(2) }; }

		inline bool Contains(const TVec2<T>& point) const
		{
			return x <= point.x && point.x <= x + w && y <= point.y && point.y <= y + h;
		}
	};

} // namespace LD