#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRender.h>
#include <Ludens/UI/UI.h>

namespace LD {
namespace UIF {

struct WidgetNode;
struct WidgetObj;

struct Widget : Handle<struct WidgetObj>
{
    typedef void (*DrawFn)(Widget widget, ScreenRenderComponent renderer);

    WidgetNode& node();

    Rect get_rect() const;

    void* get_user();

    void set_user(void* user);

    /// @brief set custom draw function to be invoked
    void set_on_draw(DrawFn drawFn);

    bool is_hovered();

    bool is_pressed();

    void on_draw(ScreenRenderComponent renderer);
};

struct PanelWidget : Widget
{
};

struct PanelWidgetInfo
{
    uint32_t color;
};

/// @brief UI button widget.
struct ButtonWidget : Widget
{
};

struct ButtonWidgetInfo
{
    const char* text;
    void (*on_press)(ButtonWidget w, MouseButton btn, void* user);
};

/// @brief UI slider widget
struct SliderWidget : Widget
{
    /// @brief get slider value
    float get_value();

    /// @brief get normalized slider ratio between 0 and 1
    float get_ratio();
};

struct SliderWidgetInfo
{
    float min; /// slider minimum value
    float max; /// slider maximum value
};

/// @brief UI toggle widget, is either in the "true" or "false" boolean state.
struct ToggleWidget : Widget
{
    /// @brief return the toggle state.
    bool get_state();
};

struct ToggleWidgetInfo
{
    void (*on_toggle)(ToggleWidget w, bool state, void* user);
    bool state; /// the state of the toggle widget when it is created
};

/// @brief UI image widget
struct ImageWidget : Widget
{
    /// @brief get target image handle
    RImage get_image();
};

struct ImageWidgetInfo
{
    RImage image;
};

struct TextWidget : Widget
{
    void set_text(const char* cstr);
};

struct TextWidgetInfo
{
    float fontSize;      /// rendered size
    const char* cstr;    /// a null terminated c string
    FontAtlas fontAtlas; /// font atlas queried during text layout, the atlas should outlive the text widget.
};

/// @brief interface to manipulate widget tree hierarchy
struct WidgetNode : Handle<struct WidgetObj>
{
    PanelWidget add_panel(const UILayoutInfo& layoutI, const PanelWidgetInfo& widgetI, void* user);

    ImageWidget add_image(const UILayoutInfo& layoutI, const ImageWidgetInfo& widgetI, void* user);

    ButtonWidget add_button(const UILayoutInfo& layoutI, const ButtonWidgetInfo& widgetI, void* user);

    SliderWidget add_slider(const UILayoutInfo& layoutI, const SliderWidgetInfo& widgetI, void* user);

    ToggleWidget add_toggle(const UILayoutInfo& layoutI, const ToggleWidgetInfo& widgetI, void* user);

    TextWidget add_text(const UILayoutInfo& layoutI, const TextWidgetInfo& widgetI, void* user);
};

} // namespace UIF
} // namespace LD