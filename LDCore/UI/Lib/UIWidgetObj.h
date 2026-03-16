#pragma once

#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UITheme.h>

#include "Widget/UIButtonWidgetObj.h"
#include "Widget/UIImageWidgetObj.h"
#include "Widget/UIPanelWidgetObj.h"
#include "Widget/UIScrollWidgetObj.h"
#include "Widget/UISliderWidgetObj.h"
#include "Widget/UITextEditWidgetObj.h"
#include "Widget/UITextWidgetObj.h"
#include "Widget/UIToggleWidgetObj.h"

namespace LD {

struct UIContextObj;
struct UIWindowObj;

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

struct UILayout
{
    UILayoutInfo info;
    Rect rect;
    float minw;
    float minh;
};

struct UICallback
{
    bool (*onEvent)(UIWidget widget, const UIEvent& event);
    void (*onUpdate)(UIWidget widget, float delta);
    void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer);
};

/// @brief UI Widget as a tagged union.
struct UIWidgetObj
{
    UILayout layout;               /// must be first field for layout semantics
    UICallback cb{};               /// callback function pointer table; TODO: single table for each type, not each instance
    UIWindowObj* window = nullptr; /// owning window
    UIWidgetObj* parent = nullptr; /// parent widget
    UIWidgetObj* child = nullptr;  /// first child widget
    UIWidgetObj* next = nullptr;   /// sibling widget
    UITheme theme{};               /// theme handle
    UINode node{};                 /// node in tree hierachy
    Vec2 scrollOffset{};           /// offset applied to children after layout
    std::string name;              /// widget debug name
    void* user = nullptr;          /// arbitrary user data
    UIWidgetType type;             /// type enum
    uint32_t flags = 0;            /// widget bit flags
    union UIWidgetUnion
    {
        UIScrollWidgetObj scroll;
        UITextWidgetObj text;
        UITextEditWidgetObj textEdit;
        UIPanelWidgetObj panel;
        UIImageWidgetObj image;
        UIButtonWidgetObj button;
        UISliderWidgetObj slider;
        UIToggleWidgetObj toggle;

        UIWidgetUnion() {}
        ~UIWidgetUnion() {}
    } as;

    UIWidgetObj() = delete;
    UIWidgetObj(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, UIWindowObj* window, void* storage, void* user);
    UIWidgetObj(const UIWidgetObj&) = delete;
    ~UIWidgetObj();

    UIWidgetObj& operator=(const UIWidgetObj&) = delete;

    /// @brief appends new child at the end of link list
    inline void append_child(UIWidgetObj* newChild)
    {
        if (!child)
        {
            child = newChild;
            return;
        }

        UIWidgetObj* last = child;
        while (last && last->next)
            last = last->next;
        last->next = newChild;
    }

    inline void remove_child(UIWidgetObj* c)
    {
        UIWidgetObj** pnext = &child;
        while (*pnext && *pnext != c)
            pnext = &(*pnext)->next;

        if (*pnext)
            *pnext = (*pnext)->next;
    }

    /// @brief get children count in linear time
    inline int get_children_count()
    {
        int count = 0;
        for (UIWidgetObj* c = child; c; c = c->next)
            count++;
        return count;
    }

    inline UIWidgetObj* get_child_by_name(const std::string& name)
    {
        for (UIWidgetObj* c = child; c; c = c->next)
        {
            if (c->name == name)
                return c;
        }

        return nullptr;
    }

    UIContextObj* ctx() const;

    /// @brief Draw the widget with default or custom render callback.
    void draw(ScreenRenderComponent renderer);
};

} // namespace LD