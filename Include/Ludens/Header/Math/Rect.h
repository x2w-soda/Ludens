#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec2.h>

namespace LD {

template <typename T>
struct TRect
{
    T x, y, w, h;

    TRect() : x(0), y(0), w(0), h(0) {}
    TRect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}

    inline TVec2<T> get_pos() const { return {x, y}; }
    inline TVec2<T> get_size() const { return {w, h}; }

    inline void get_pos(T& x_, T& y_) const
    {
        x_ = x;
        y_ = y;
    }

    inline void get_size(T& w_, T& h_) const
    {
        w_ = w;
        h_ = h;
    }

    /// @brief check whether the rect contains a point
    /// @param pos a point position in 2D coordinates
    inline bool contains(const TVec2<T>& pos) const { return x <= pos.x && pos.x <= x + w && y <= pos.y && pos.y <= y + h; }

    /// @brief two rects are equal if they have the same position and size. Epsilon tolerance is used for floating-point comparison.
    bool operator==(const TRect& other) const
    {
        return is_equal_epsilon(x, other.x) && is_equal_epsilon(y, other.y) && is_equal_epsilon(w, other.w) && is_equal_epsilon(h, other.h);
    }
};

using Rect = TRect<float>;
using IRect = TRect<int>;

} // namespace LD