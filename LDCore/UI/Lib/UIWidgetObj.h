#pragma once

#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UITheme.h>

#include "UILibDef.h"

#include "Widget/UIButtonWidgetObj.h"
#include "Widget/UIDropdownWidgetObj.h"
#include "Widget/UIImageWidgetObj.h"
#include "Widget/UIPanelWidgetObj.h"
#include "Widget/UIScrollBarWidgetObj.h"
#include "Widget/UIScrollWidgetObj.h"
#include "Widget/UISliderWidgetObj.h"
#include "Widget/UITextEditWidgetObj.h"
#include "Widget/UITextWidgetObj.h"
#include "Widget/UIToggleWidgetObj.h"

namespace LD {

struct UIContextObj;
struct UIWindowObj;
struct UIWidgetObj;

/// @brief UI Widget as a tagged union.
union UIWidgetUnion
{
    UIScrollWidgetObj scroll;
    UIScrollBarWidgetObj scrollBar;
    UITextWidgetObj text;
    UITextEditWidgetObj textEdit;
    UIPanelWidgetObj panel;
    UIImageWidgetObj image;
    UIButtonWidgetObj button;
    UISliderWidgetObj slider;
    UIToggleWidgetObj toggle;
    UIDropdownWidgetObj dropdown;

    UIWidgetUnion() {}
    ~UIWidgetUnion() {}
};

/// @brief Full UI Widget information.
struct UIWidgetObj
{
    UIWidgetType type;             /// type enum
    UIID id = {};                  /// widget ID unique throught context
    UIWidgetLayout* L = nullptr;   /// widget layout information
    UIWidgetUnion* U = nullptr;    /// widget type-specific information
    UIWindowObj* window = nullptr; /// owning window
    UIWidgetObj* parent = nullptr; /// parent widget
    UIWidgetObj* child = nullptr;  /// first child widget
    UIWidgetObj* next = nullptr;   /// sibling widget
    UIWidgetCallback userCB = {};  /// callback function pointer table
    UITheme theme = {};            /// theme handle
    std::string name;              /// widget debug name
    void* data = nullptr;          /// widget type data
    void* user = nullptr;          /// arbitrary user data
    uint32_t flags = 0;            /// widget bit flags

    UIWidgetObj() = delete;
    UIWidgetObj(UIWidgetType type, UIContextObj* ctx, UIWidgetLayout* widgetL, UIWidgetUnion* widgetU, UIWidgetObj* parent, UIWindowObj* window, void* storage, void* user);
    UIWidgetObj(const UIWidgetObj&) = delete;
    ~UIWidgetObj();

    UIWidgetObj& operator=(const UIWidgetObj&) = delete;

    /// @brief appends new child at the end of link list
    void append_child(UIWidgetObj* newChild);
    void remove_child(UIWidgetObj* c);

    /// @brief get children count in linear time
    int get_children_count();
    UIWidgetObj* get_child_by_name(const std::string& name);
    Rect get_child_rect_union();

    UIContextObj* ctx() const;

    /// @brief Draw the widget with default or custom render callback.
    void draw(ScreenRenderComponent renderer);
};

} // namespace LD