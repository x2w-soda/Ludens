#pragma once

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

    inline bool contains(const TVec2<T>& pos) const { return x <= pos.x && pos.x <= x + w && y <= pos.y && pos.y <= y + h; }
};

using Rect = TRect<float>;
using IRect = TRect<int>;

} // namespace LD