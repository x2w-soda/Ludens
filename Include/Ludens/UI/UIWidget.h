#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UITheme.h>

namespace LD {

struct UINode;
struct UIContextObj;

enum UIEvent
{
    UI_MOUSE_ENTER,
    UI_MOUSE_LEAVE,
    UI_MOUSE_DOWN,
    UI_MOUSE_UP,
    UI_KEY_DOWN,
    UI_KEY_UP,
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

    /// @brief This widget will silently block input events
    ///        without propagating to subtree.
    void block_input();

    /// @brief This widget subtree will receive input events normally.
    void unblock_input();

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

    /// @brief whether the widget is being pressed and not yet released
    bool is_pressed();

    /// @brief get user data pointer
    void* get_user();

    /// @brief set user data pointer
    void set_user(void* user);

    /// @brief Get current widget layout.
    void get_layout(UILayoutInfo& layout);

    /// @brief Update widget layout after creation.
    void set_layout(const UILayoutInfo& layout);

    /// @brief Update widget sizing policy after creation.
    void set_layout_size(const UISize& sizeX, const UISize& sizeY);

    /// @brief Update widget child padding after creation.
    void set_layout_child_padding(const UIPadding& padding);

    /// @brief Update widget child gap after creation.
    void set_layout_child_gap(float gap);

    /// @brief Update widget child axis after creation.
    void set_layout_child_axis(UIAxis axis);

    /// @brief Update widget child alignment along X axis.
    void set_layout_child_align_x(UIAlign childAlignX);

    /// @brief Update widget child alignment along Y axis.
    void set_layout_child_align_y(UIAlign childAlignY);

    /// @brief override key callback
    void set_on_key(void (*onKey)(UIWidget widget, KeyCode key, UIEvent event));

    /// @brief override mouse callback
    void set_on_mouse(void (*onMouse)(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event));

    /// @brief override mouse hover callback
    void set_on_hover(void (*onHover)(UIWidget widget, UIEvent event));

    /// @brief override mouse drag callback
    void set_on_drag(void (*onDrag)(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin));

    /// @brief Override scroll callback.
    void set_on_scroll(void (*onScroll)(UIWidget widget, const Vec2& offset));

    /// @brief override widget update callback
    void set_on_update(void (*onUpdate)(UIWidget widget, float delta));

    /// @brief override widget draw callback
    void set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer));
};

struct UIScrollWidget : UIWidget
{
    /// @brief Set scroll offset along X axis.
    void set_scroll_offset_x(float offset);

    /// @brief Set scroll offset along Y axis.
    void set_scroll_offset_y(float offset);

    /// @brief Update scroll background color
    void set_scroll_bg_color(Color color);

    /// @brief Default scroll widget update for smooth scrolling.
    static void on_update(UIWidget widget, float delta);

    /// @brief Default scroll widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIScrollWidgetInfo
{
    Color bgColor;
};

struct UIPanelWidget : UIWidget
{
    /// @brief Access panel color.
    Color* panel_color();

    /// @brief Default panel widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIPanelWidgetInfo
{
    Color color;
};

/// @brief UI button widget.
struct UIButtonWidget : UIWidget
{
    /// @brief Default button widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIButtonWidgetInfo
{
    const char* text;
    Color textColor;
    void (*on_press)(UIButtonWidget w, MouseButton btn, void* user);
    bool transparentBG;
};

/// @brief UI slider widget
struct UISliderWidget : UIWidget
{
    /// @brief Set slider value range.
    void set_value_range(float minValue, float maxValue);

    /// @brief get slider value
    float get_value();

    /// @brief get normalized slider ratio between 0 and 1
    float get_ratio();

    /// @brief Default slider widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UISliderWidgetInfo
{
    float min; /// slider minimum value
    float max; /// slider maximum value
};

/// @brief UI toggle widget, is either in the "true" or "false" boolean state.
struct UIToggleWidget : UIWidget
{
    /// @brief return the toggle state.
    bool get_state();

    /// @brief Default image widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIToggleWidgetInfo
{
    void (*on_toggle)(UIToggleWidget w, bool state, void* user);
    bool state; /// the state of the toggle widget when it is created
};

/// @brief UI image widget
struct UIImageWidget : UIWidget
{
    /// @brief get target image handle
    RImage get_image();

    /// @brief Set rect in image to display.
    void set_image_rect(const Rect& rect);

    /// @brief Get displayed rect in image.
    Rect get_image_rect();

    /// @brief Set image tint color.
    void set_image_tint(Color color);

    /// @brief Default image widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIImageWidgetInfo
{
    RImage image;     /// image to be renderered
    const Rect* rect; /// if not null, the part of image to be rendered
};

struct UITextWidget : UIWidget
{
    /// @brief Set text content.
    void set_text(const char* cstr);

    /// @brief Get text content. Treat return value as a transient pointer.
    const char* get_text();

    /// @brief Access text font size.
    float* font_size();

    /// @brief Default text widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UITextWidgetInfo
{
    float fontSize;   /// rendered size
    const char* cstr; /// a null terminated c string
    bool hoverHL;     /// whether to highlight the text when hovered
    Color* bgColor;   /// if not null, the background color under text
};

struct UITextEditWidget : UIWidget
{
    /// @brief Default text edit widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UITextEditWidgetInfo
{
    float fontSize;          /// rendered size
    const char* placeHolder; /// default gray text to display when empty
};

/// @brief interface to manipulate widget tree hierarchy
struct UINode : Handle<struct UIWidgetObj>
{
    UIContextObj* get_context();

    void get_children(std::vector<UIWidget>& widgets);

    /// @brief Remove self subtree from parent.
    /// @warning All UIWidget handle in the removed subtree is now out of date.
    void remove();

    UIScrollWidget add_scroll(const UILayoutInfo& layoutI, const UIScrollWidgetInfo& widgetI, void* user);

    UIPanelWidget add_panel(const UILayoutInfo& layoutI, const UIPanelWidgetInfo& widgetI, void* user);

    UIImageWidget add_image(const UILayoutInfo& layoutI, const UIImageWidgetInfo& widgetI, void* user);

    UIButtonWidget add_button(const UILayoutInfo& layoutI, const UIButtonWidgetInfo& widgetI, void* user);

    UISliderWidget add_slider(const UILayoutInfo& layoutI, const UISliderWidgetInfo& widgetI, void* user);

    UIToggleWidget add_toggle(const UILayoutInfo& layoutI, const UIToggleWidgetInfo& widgetI, void* user);

    UITextWidget add_text(const UILayoutInfo& layoutI, const UITextWidgetInfo& widgetI, void* user);

    UITextEditWidget add_text_edit(const UILayoutInfo& layoutI, const UITextEditWidgetInfo& widgetI, void* user);
};

/// @brief Get static C string for widget type.
const char* get_ui_widget_type_cstr(UIWidgetType type);

/// @brief Try get widget type from C string.
/// @return True on success.
bool get_ui_widget_type_from_cstr(UIWidgetType& outType, const char* cstr);

} // namespace LD