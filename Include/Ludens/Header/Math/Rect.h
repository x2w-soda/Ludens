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

    /// @brief Split a rect vertically into left, right, and split area.
    /// @param ratio Normalized split ratio.
    /// @param splitWidth If positive, the width of the split area.
    /// @param area Original area before split.
    /// @param left Outputs left area after split.
    /// @param right Outputs right area after split.
    /// @param splitArea Outputs split area after split.
    static void split_v(float ratio, T splitWidth, const TRect& area, TRect& left, TRect& right, TRect& splitArea)
    {
        left = area;
        left.w = area.w * ratio;
        left.w -= splitWidth / static_cast<T>(2);

        splitArea = TRect(left.x + left.w, left.y, splitWidth, left.h);

        right = area;
        right.x += left.w + splitWidth;
        right.w = area.w * (static_cast<T>(1) - ratio) - splitWidth / static_cast<T>(2);
    }

    /// @brief Split a rect vertically into left and right area.
    static inline void split_v(float ratio, const TRect& area, TRect& left, TRect& right)
    {
        TRect splitArea; // zero area
        return TRect::split_v(ratio, (T)0, area, left, right, splitArea);
    }

    /// @brief Split a rect horizontally into top, bottom, and split area.
    /// @param ratio Normalized split ratio.
    /// @param splitHeight If positive, the height of the split area.
    /// @param area Original area before split.
    /// @param top Outputs top area after split.
    /// @param bottom Outputs bottom area after split.
    /// @param splitArea Outputs split area after split.
    static void split_h(float ratio, T splitHeight, const TRect& area, TRect& top, TRect& bottom, TRect& splitArea)
    {
        top = area;
        top.h = area.h * ratio;
        top.h -= splitHeight / static_cast<T>(2);

        splitArea = TRect(top.x, top.y + top.h, top.w, splitHeight);

        bottom = area;
        bottom.y += top.h + splitHeight;
        bottom.h = area.h * (static_cast<T>(1) - ratio) - splitHeight / static_cast<T>(2);
    }

    /// @brief Split a rect horizontally into top and bottom area.
    static inline void split_h(float ratio, const TRect& area, TRect& top, TRect& bottom)
    {
        TRect splitArea; // zero area
        return TRect::split_h(ratio, (T)0, area, top, bottom, splitArea);
    }

    /// @brief Scale rect width while preserving original center.
    /// @param ratio Positive ratio to scale rect width. 
    /// @return Scaled rect with new width, or the original rect if ratio is invalid.
    static inline TRect scale_w(const TRect& area, float ratio)
    {
        if (ratio <= 0.0f)
            return area;

        const T oldW = area.w;
        const T newW = area.w * (T)ratio;
        return TRect(area.x - (newW - oldW) / (T)2, area.y, newW, area.h);
    }

    /// @brief Scale rect height while preserving original center.
    /// @param ratio Positive ratio to scale rect height.
    /// @return Scaled rect with new height, or the original rect if ratio is invalid.
    static inline TRect scale_h(const TRect& area, float ratio)
    {
        if (ratio <= 0.0f)
            return area;

        const T oldH = area.h;
        const T newH = area.h * (T)ratio;
        return TRect(area.x, area.y - (newH - oldH) / (T)2, area.w, newH);
    }
};

using Rect = TRect<float>;
using IRect = TRect<int>;

} // namespace LD