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

/// @brief wrap sizing callback, given length limit in main axis,
///        user returns the result size on the secondary axis after wrapping.
typedef float (*UIWrapSizeFn)(UIWidgetObj* widget, float inLimit);

/// @brief wrap limit callback, user outputs minimum extent of the wrappable
///        and the maximum extent if unwrapped.
typedef void (*UIWrapLimitFn)(UIWidgetObj* widget, float& outMin, float& outMax);

struct UISize
{
    UIWrapSizeFn wrapSizeFn;
    UIWrapLimitFn wrapLimitFn;
    UISizeType type;
    float extent;

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
    static inline UISize wrap_primary(UIWrapSizeFn sizeFn, UIWrapLimitFn limitFn)
    {
        return {.wrapSizeFn = sizeFn, .wrapLimitFn = limitFn, .type = UI_SIZE_WRAP_PRIMARY, .extent = 0.0f};
    }

    static inline UISize wrap_secondary()
    {
        return {.type = UI_SIZE_WRAP_SECONDARY};
    }

    /// @brief declare fixed size for this UI node
    static inline UISize fixed(float extent)
    {
        return {.type = UI_SIZE_FIXED, .extent = extent};
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