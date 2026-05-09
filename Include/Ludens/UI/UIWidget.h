#pragma once

#include <Ludens/DSA/IDRegistry.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIDef.h>
#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UITheme.h>

namespace LD {

using UIID = ID;

/// @brief UIWidget handle, this contains all the base methods of a widget.
struct UIWidget : Handle<struct UIWidgetObj>
{
    /// @brief Get widget type.
    UIWidgetType get_type();

    /// @brief Get widget unique ID throught UIContext.
    UIID get_id();

    struct UIContextObj* get_context_obj();

    void* get_data();
    void set_data(void* data);
    void* get_user();
    void set_user(void* user);

public: // events and state
    /// @brief whether the widget is under the mouse cursor
    bool is_hovered();

    /// @brief Whether the widget is the global focused widget in context.
    bool is_focused();

    /// @brief whether the widget is being pressed and not yet released
    bool is_pressed();

    /// @brief whether the widget is being dragged and not yet released
    bool is_dragged();

    /// @brief User widget update callback
    void set_on_update(void (*onUpdate)(UIWidget widget, float delta));

    /// @brief User widget draw callback
    void set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer));

    /// @brief User widget event handler callback
    void set_on_event(bool (*onEvent)(UIWidget widget, const UIEvent& event));
    bool has_on_event();

    void set_consume_mouse_event(bool consumes);
    void set_consume_key_event(bool consumes);
    void set_consume_scroll_event(bool consumes);

public: // hierarchy and identity
    void get_name(String& name);
    void set_name(View name);
    UIWidget add_child(UIWidgetType type, const UILayoutInfo& layoutI, void* data, void* user);
    UIWidget get_child_by_name(View childName);
    void get_children(std::vector<UIWidget>& widgets);

    /// @brief Remove self subtree from parent.
    /// @warning All UIWidget handle in the removed subtree is now out of date.
    void remove();

    /// @brief Remove all children from self.
    /// @warning All UIWidget handles of the children subtrees are now out of date.
    void remove_children();

public: // layout and visibility
    /// @brief If hidden, skips rendering for all UIWidgets in this subtree.
    /// @note Child widgets also have their own visbility mask, applied to their respective subtrees.
    void set_visible(bool isVisible);
    inline void hide() { set_visible(false); }
    inline void show() { set_visible(true); }

    /// @brief Check if widget subtree is visible.
    bool is_visible();

    /// @brief Get widget rect in screen space.
    Rect get_rect();

    /// @brief Get widget position in screen space.
    Vec2 get_pos();

    /// @brief Get widget extent in screen space.
    Vec2 get_size();

    /// @brief Get union of children rects in screen space.
    Rect get_child_rect_union();

    /// @brief get mouse position relative to widget origin
    /// @param pos output mouse position if mouse is within widget
    /// @return true if mouse is within widget rect
    bool get_mouse_pos(Vec2& pos);

    /// @brief Get UI theme handle.
    UITheme get_theme();

    Color get_state_color(Color base);

    /// @brief Get current widget layout.
    void get_layout(UILayoutInfo& layout);

    /// @brief Update widget layout after creation.
    void set_layout(const UILayoutInfo& layout);

    /// @brief Update widget sizing policy after creation.
    void set_layout_size(UISize sizeX, UISize sizeY);
    void set_layout_size_x(UISize sizeX);
    void set_layout_size_y(UISize sizeY);

    /// @brief Update widget child padding after creation.
    void set_layout_child_padding(const UIPadding& childPad);

    /// @brief Update widget child gap after creation.
    void set_layout_child_gap(float childGap);

    /// @brief Update widget child axis after creation.
    void set_layout_child_axis(UIAxis axis);

    /// @brief Update widget child alignment along X axis.
    void set_layout_child_align_x(UIAlign childAlignX);

    /// @brief Update widget child alignment along Y axis.
    void set_layout_child_align_y(UIAlign childAlignY);

public: // static methods
    /// @brief Get static C string for widget type.
    static const char* get_type_cstr(UIWidgetType type);

    /// @brief Get default layout for widget type.
    static UILayoutInfo get_default_layout(UIWidgetType type);

    /// @brief Try get widget type from C string.
    /// @return True on success.
    static bool get_type_from_cstr(UIWidgetType& outType, const char* cstr);
};

} // namespace LD