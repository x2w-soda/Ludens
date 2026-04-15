#pragma once

#include <Ludens/UI/UIDef.h>

namespace LD {

struct UIPadding
{
    float left;
    float right;
    float top;
    float bottom;

    UIPadding() = default;
    UIPadding(float pad)
        : left(pad), right(pad), top(pad), bottom(pad) {}
    UIPadding(float leftPad, float rightPad, float topPad, float botPad)
        : left(leftPad), right(rightPad), top(topPad), bottom(botPad) {}

    static inline UIPadding left_right(float left, float right)
    {
        return UIPadding(left, right, 0.0f, 0.0f);
    };

    static inline UIPadding top_bottom(float top, float bot)
    {
        return UIPadding(0.0f, 0.0f, top, bot);
    };
};

struct UISize
{
    float extent;
    UISizeType type;

    /// @brief determine size to fit children tightly
    static inline UISize fit()
    {
        return {.extent = 0.0f, .type = UI_SIZE_FIT};
    }

    /// @brief expand to take up space in container
    static inline UISize grow()
    {
        return {.extent = 0.0f, .type = UI_SIZE_GROW};
    }

    /// @brief wrap around and grow along the other axis
    static inline UISize wrap()
    {
        return {.extent = 0.0f, .type = UI_SIZE_WRAP};
    }

    /// @brief declare fixed size for this UI node
    static inline UISize fixed(float extent)
    {
        return {.extent = extent, .type = UI_SIZE_FIXED};
    }
};

/// @brief the layout properties of a UI node.
struct UILayoutInfo
{
    /// @brief size layout policy along the horizontal axis
    UISize sizeX;

    /// @brief size layout policy along the vertical axis
    UISize sizeY;

    /// @brief gap between self and children nodes
    UIPadding childPadding;

    /// @brief the gap between child nodes
    float childGap;

    /// @brief which direction to align children
    UIAxis childAxis;

    /// @brief Alignment of children along X axis.
    UIAlign childAlignX;

    /// @brief Alignment of children along Y axis.
    UIAlign childAlignY;

    UILayoutInfo() = default;
    UILayoutInfo(UISize x, UISize y)
        : sizeX(x), sizeY(y), childPadding{}, childGap(0.0f), childAxis(), childAlignX(), childAlignY()
    {
    }
};

} // namespace LD