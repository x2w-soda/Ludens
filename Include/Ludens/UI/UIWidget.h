#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/View.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UITheme.h>

namespace LD {

struct UINode;
struct UIContextObj;

enum UIEventType
{
    UI_EVENT_SCROLL,
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_MOUSE_POSITION,
    UI_EVENT_MOUSE_DOWN,
    UI_EVENT_MOUSE_UP,
    UI_EVENT_MOUSE_DRAG,
    UI_EVENT_MOUSE_ENTER,
    UI_EVENT_MOUSE_LEAVE,
    UI_EVENT_FOCUS_ENTER,
    UI_EVENT_FOCUS_LEAVE,
};

struct UIEvent
{
    UIEventType type;

    union
    {
        struct
        {
            KeyMods mods;
            KeyCode code;
        } key;

        struct
        {
            KeyMods mods;
            MouseButton button;
            Vec2 position;
        } mouse;

        struct
        {
            Vec2 offset;
        } scroll;

        struct
        {
            MouseButton button;
            Vec2 position;
            bool begin;
        } drag;
    };
};

enum UIWidgetType
{
    UI_WIDGET_WINDOW = 0,
    UI_WIDGET_SCROLL,
    UI_WIDGET_BUTTON,
    UI_WIDGET_SLIDER,
    UI_WIDGET_TOGGLE,
    UI_WIDGET_PANEL,
    UI_WIDGET_IMAGE,
    UI_WIDGET_TEXT,
    UI_WIDGET_TEXT_EDIT,
    UI_WIDGET_TYPE_COUNT,
};

struct UIWidget : Handle<struct UIWidgetObj>
{
    /// @brief Get node in widget hierarchy.
    UINode& node();

    /// @brief If hidden, skips rendering for all UIWidgets in this subtree.
    /// @note Child widgets also have their own visbility mask, applied to their respective subtrees.
    void set_visible(bool isVisible);

    inline void hide() { set_visible(false); }
    inline void show() { set_visible(true); }

    /// @brief Check if widget subtree is visible.
    bool is_visible();

    /// @brief Get widget type.
    UIWidgetType get_type();

    /// @brief Get widget rect in screen space.
    Rect get_rect();

    /// @brief Get widget position in screen space.
    Vec2 get_pos();

    /// @brief Get widget extent in screen space.
    Vec2 get_size();

    /// @brief Get UI theme handle.
    UITheme get_theme();

    /// @brief get mouse position relative to widget origin
    /// @param pos output mouse position if mouse is within widget
    /// @return true if mouse is within widget rect
    bool get_mouse_pos(Vec2& pos);

    /// @brief whether the widget is under the mouse cursor
    bool is_hovered();

    /// @brief Whether the widget is the global focused widget in context.
    bool is_focused();

    /// @brief whether the widget is being pressed and not yet released
    bool is_pressed();

    /// @brief whether the widget is being dragged and not yet released
    bool is_dragged();

    /// @brief get user data pointer
    void* get_user();

    /// @brief set user data pointer
    void set_user(void* user);

    /// @brief override widget update callback
    void set_on_update(void (*onUpdate)(UIWidget widget, float delta));

    /// @brief override widget draw callback
    void set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer));

    /// @brief override widget event handler callback
    void set_on_event(bool (*onEvent)(UIWidget widget, const UIEvent& event));
    bool has_on_event();

    void get_name(std::string& name);

    void set_name(const std::string& name);

    void set_consume_mouse_event(bool consumes);
    void set_consume_key_event(bool consumes);
    void set_consume_scroll_event(bool consumes);

    UIWidget get_child_by_name(const std::string& childName);

    /// @brief Get current widget layout.
    void get_layout(UILayoutInfo& layout);

    /// @brief Update widget layout after creation.
    void set_layout(const UILayoutInfo& layout);

    /// @brief Update widget sizing policy after creation.
    void set_layout_size(const UISize& sizeX, const UISize& sizeY);

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
};

struct UIToggleStorage;
struct UITextStorage;
struct UISliderStorage;
struct UIButtonStorage;
struct UIImageStorage;
struct UIPanelStorage;
struct UITextEditStorage;
struct UIScrollStorage;

/// @brief interface to manipulate widget tree hierarchy
struct UINode : Handle<struct UIWidgetObj>
{
    UIContextObj* get_context();

    void get_children(std::vector<UIWidget>& widgets);

    /// @brief Remove self subtree from parent.
    /// @warning All UIWidget handle in the removed subtree is now out of date.
    void remove();

    UIWidget add_scroll(const UILayoutInfo& layoutI, UIScrollStorage* storage, void* user);
    UIWidget add_panel(const UILayoutInfo& layoutI, UIPanelStorage* storage, void* user);
    UIWidget add_image(const UILayoutInfo& layoutI, UIImageStorage* storage, void* user);
    UIWidget add_button(const UILayoutInfo& layoutI, UIButtonStorage* storage, void* user);
    UIWidget add_slider(const UILayoutInfo& layoutI, UISliderStorage* storage, void* user);
    UIWidget add_toggle(const UILayoutInfo& layoutI, UIToggleStorage* storage, void* user);
    UIWidget add_text(const UILayoutInfo& layoutI, UITextStorage* storage, void* user);
    UIWidget add_text_edit(const UILayoutInfo& layoutI, UITextEditStorage* storage, void* user);
};

/// @brief Get static C string for widget type.
const char* get_ui_widget_type_cstr(UIWidgetType type);

/// @brief Try get widget type from C string.
/// @return True on success.
bool get_ui_widget_type_from_cstr(UIWidgetType& outType, const char* cstr);

} // namespace LD