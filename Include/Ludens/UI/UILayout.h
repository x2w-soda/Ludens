#pragma once

#include <Ludens/Header/Directional.h>

namespace LD {

struct UIWidgetObj;

enum UIAxis
{
    UI_AXIS_X = AXIS_X,
    UI_AXIS_Y = AXIS_Y,
};

enum UISizeType
{
    UI_SIZE_FIT = 0,
    UI_SIZE_GROW,
    UI_SIZE_FIXED,
    UI_SIZE_WRAP_PRIMARY,
    UI_SIZE_WRAP_SECONDARY,
};

struct UIPadding
{
    float left;
    float right;
    float top;
    float bottom;
};

struct UISize
{
    float extent;
    UISizeType type;

    /// @brief determine size to fit children tightly
    static inline UISize fit()
    {
        return {.type = UI_SIZE_FIT};
    }

    /// @brief expand to take up space in container
    static inline UISize grow()
    {
        return {.type = UI_SIZE_GROW};
    }

    /// @brief if text does not fit into main axis, wrap around and grow along the secondary axis
    static inline UISize wrap_primary()
    {
        return {.type = UI_SIZE_WRAP_PRIMARY};
    }

    static inline UISize wrap_secondary()
    {
        return {.type = UI_SIZE_WRAP_SECONDARY};
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
    UILayoutInfo() = default;

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
};

} // namespace LD