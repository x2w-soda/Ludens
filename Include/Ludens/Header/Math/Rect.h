#pragma once

#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec2.h>

namespace LD {

template <typename T>
struct TRect
{
    T x, y, w, h;

    TRect() = default;
    TRect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}

    /// @brief Get top left corner position
    inline TVec2<T> get_pos() const { return {x, y}; }

    /// @brief Get top right corner position
    inline TVec2<T> get_pos_tr() const { return {x + w, y}; }

    /// @brief Get bottom right corner position
    inline TVec2<T> get_pos_br() const { return {x + w, y + h}; }

    /// @brief Get bottom left corner position
    inline TVec2<T> get_pos_bl() const { return {x, y + h}; }

    /// @brief Get rect extent.
    inline TVec2<T> get_size() const { return {w, h}; }

    /// @brief Get center position in rect.
    inline TVec2<T> get_center() const { return {x + w / (T)2, y + h / (T)2}; }

    /// @brief Get rect position.
    inline void get_pos(T& x_, T& y_) const
    {
        x_ = x;
        y_ = y;
    }

    /// @brief Set rect position.
    inline void set_pos(T x_, T y_)
    {
        x = x_;
        y = y_;
    }

    /// @brief Get rect size.
    inline void get_size(T& w_, T& h_) const
    {
        w_ = w;
        h_ = h;
    }

    /// @brief Set rect size.
    inline void set_size(T w_, T h_)
    {
        w = w_;
        h = h_;
    }

    /// @brief Get euclidean distance from a point to rect center.
    /// @param pos A point in 2D space.
    /// @return Distance from point to rect center
    T get_center_distance(const TVec2<T>& pos) const
    {
        return (pos - get_center()).length();
    }

    /// @brief Query distances from a point to rect edge.
    /// @param left If not null, outputs the distance to left rect edge.
    /// @param top If not null, outputs the distance to top rect edge.
    /// @param right If not null, outputs the distance to right rect edge.
    /// @param bottom If not null, outputs the distance to bottom rect edge.
    void get_edge_distances(const TVec2<T>& pos, T* left, T* top, T* right, T* bot) const
    {
        if (left)
            *left = LD_ABS(pos.x - x);
        if (top)
            *top = LD_ABS(pos.y - y);
        if (right)
            *right = LD_ABS(pos.x - (x + w));
        if (bot)
            *bot = LD_ABS(pos.y - (y + h));
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