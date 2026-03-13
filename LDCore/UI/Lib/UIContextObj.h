#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UILayerObj;
struct UIWidgetObj;
struct UIWorkspaceObj;
struct UIEvent;

/// @brief UI context implementation.
struct UIContextObj
{
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    PoolAllocator widgetPA;
    UITheme theme;
    Vector<UILayerObj*> layers;
    HashSet<UILayerObj*> deferredLayerDestruction;
    UIWidgetObj* dragWidget = nullptr;      /// the widget begin dragged
    UIWidgetObj* pressWidget = nullptr;     /// the widget pressed and not yet released
    UIWidgetObj* focusWidget = nullptr;     /// the widget receiving key events
    UIWidgetObj* hoverWidgetLeaf = nullptr; /// the leaf widget under mouse cursor accepting events
    HashSet<UIWidgetObj*> hoverWidgets;     /// set of all widgets under cursor
    void* user;
    void (*onEvent)(UIWidget, const UIEvent&, void*) = nullptr;
    Vec2 cursorPos;              /// mouse cursor global position
    Vec2 dragStartPos;           /// mouse cursor drag start global position
    MouseButton dragMouseButton; /// mouse button used for dragging

    UIWidgetObj* alloc_widget(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, void* user);
    void free_widget(UIWidgetObj* widget);

    UIWidgetObj* get_widget(const Vec2& pos);
    UIWidgetObj* get_widget_in_layer(UILayerObj* layer, const Vec2& pos);
    UIWidgetObj* get_widget_in_workspace(UIWorkspaceObj* space, const Vec2& pos);

    void pre_update();

    UILayerObj* get_or_create_layer(const char* name);
    UILayerObj* get_layer(const char* name);
    void raise_layer(UILayerObj* obj);

    /// @brief When a widget is removed, reset all references to it.
    void invalidate_refs(UIWidgetObj* removed);

    /// @brief Assign global focus widget, nullptr clears focus state.
    void focus_widget(UIWidgetObj* nextFocusWidget);

    /// @brief Assign next global hover widget leaf, updates the set of hovered widgets.
    void hover_widget(UIWidgetObj* nextHoverLeafWidget);

    bool input_mouse_position(const UIEvent& event);
    bool input_mouse_down(const UIEvent& event);
    bool input_mouse_up(const UIEvent& event);
    bool input_key(const UIEvent& event);
    bool input_scroll(const UIEvent& event);
};

} // namespace LD