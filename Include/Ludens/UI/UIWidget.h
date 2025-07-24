#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRender.h>
#include <Ludens/UI/UILayout.h>

namespace LD {

struct UINode;

struct UIWidget : Handle<struct UIWidgetObj>
{
    // get node in widget hierarchy
    UINode& node();

    /// @brief get widget rect in screen space
    Rect get_rect();

    /// @brief whether the widget is under the mouse cursor
    bool is_hovered();

    /// @brief whether the widget is being pressed and not yet released
    bool is_pressed();

    /// @brief draw the widget
    void on_draw(ScreenRenderComponent renderer);

    /// @brief get user data pointer
    void* get_user();

    /// @brief set user data pointer
    void set_user(void* user);

    /// @brief override key up callback
    void set_on_key_up(void (*onKeyUp)(UIWidget widget, KeyCode key));

    /// @brief override key down callback
    void set_on_key_down(void (*onKeyDown)(UIWidget widget, KeyCode key));

    /// @brief override mouse up callback
    void set_on_mouse_up(void (*onMouseUp)(UIWidget widget, MouseButton btn));

    /// @brief override mouse down callback
    void set_on_mouse_down(void (*onMouseDown)(UIWidget widget, MouseButton btn));

    /// @brief override mouse enter callback
    void set_on_enter(void (*onEnter)(UIWidget widget));

    /// @brief override mouse leave callback
    void set_on_leave(void (*onLeave)(UIWidget widget));

    /// @brief override mouse drag callback
    void set_on_drag(void (*onDrag)(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin));

    /// @brief override widget update callback
    void set_on_update(void (*onUpdate)(UIWidget widget, float delta));

    /// @brief override widget draw callback
    void set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer));
};

struct UIPanelWidget : UIWidget
{
};

struct UIPanelWidgetInfo
{
    uint32_t color;
};

/// @brief UI button widget.
struct UIButtonWidget : UIWidget
{
};

struct UIButtonWidgetInfo
{
    const char* text;
    void (*on_press)(UIButtonWidget w, MouseButton btn, void* user);
};

/// @brief UI slider widget
struct UISliderWidget : UIWidget
{
    /// @brief get slider value
    float get_value();

    /// @brief get normalized slider ratio between 0 and 1
    float get_ratio();
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
};

struct UIImageWidgetInfo
{
    RImage image;
};

struct UITextWidget : UIWidget
{
    void set_text(const char* cstr);
};

struct UITextWidgetInfo
{
    float fontSize;      /// rendered size
    const char* cstr;    /// a null terminated c string
    FontAtlas fontAtlas; /// font atlas queried during text layout, the atlas should outlive the text widget.
};

/// @brief interface to manipulate widget tree hierarchy
struct UINode : Handle<struct UIWidgetObj>
{
    UIPanelWidget add_panel(const UILayoutInfo& layoutI, const UIPanelWidgetInfo& widgetI, void* user);

    UIImageWidget add_image(const UILayoutInfo& layoutI, const UIImageWidgetInfo& widgetI, void* user);

    UIButtonWidget add_button(const UILayoutInfo& layoutI, const UIButtonWidgetInfo& widgetI, void* user);

    UISliderWidget add_slider(const UILayoutInfo& layoutI, const UISliderWidgetInfo& widgetI, void* user);

    UIToggleWidget add_toggle(const UILayoutInfo& layoutI, const UIToggleWidgetInfo& widgetI, void* user);

    UITextWidget add_text(const UILayoutInfo& layoutI, const UITextWidgetInfo& widgetI, void* user);
};

} // namespace LD