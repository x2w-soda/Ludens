#pragma once

#include <Ludens/UI/UILayout.h>

namespace LD {

struct UIWidgetObj;

enum UIWidgetFlagBit
{
    /// @brief Widget subtree will not be drawn.
    UI_WIDGET_FLAG_HIDDEN_BIT = LD_BIT(0),

    /// @brief Widget storage is allocated locally and has same lifetime as widget.
    UI_WIDGET_FLAG_LOCAL_STORAGE_BIT = LD_BIT(1),

    /// @brief Widget subtree will be drawn with scissor.
    UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT = LD_BIT(2),

    /// @brief Widget can receive focus signals.
    UI_WIDGET_FLAG_FOCUSABLE_BIT = LD_BIT(3),

    /// @brief Widget 'handles' mouse events without an actual event handler function.
    UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT = LD_BIT(4),

    /// @brief Widget 'handles' key events without an actual event handler function.
    UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT = LD_BIT(5),

    /// @brief Widget 'handles' scroll events without an actual event handler function.
    UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT = LD_BIT(6),
};

struct UIWidgetLayout
{
    UILayoutInfo info;     // layout intent info
    Vec2 childOffset = {}; // offset applied to children
    Rect rect;             // final global rect calculated from layout
    float minw;
    float minh;
};

struct UIWidgetCallback
{
    bool (*onEvent)(UIWidget widget, const UIEvent& event);
    void (*onUpdate)(UIWidget widget, float delta);
    void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer);
};

template <typename TData>
struct UIWidgetBaseObj
{
    UIWidgetObj* base;
    TData local;

    /// @brief Widget data can be allocated locally or externally,
    ///        ensure UIWidgetObj::data points to valid storage.
    inline void connect(UIWidgetObj* baseObj)
    {
        base = baseObj;

        if (!base->data)
        {
            base->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
            base->data = &local;
        }
    }

    inline const TData& get_data() const
    {
        return *(const TData*)base->data;
    }

    inline TData& get_data()
    {
        return *(TData*)base->data;
    }

    inline const Rect& get_rect() const
    {
        return base->L->rect;
    }
};

} // namespace LD