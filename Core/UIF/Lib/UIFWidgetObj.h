#pragma once

#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UI.h>
#include <Ludens/UIF/UIFAnimation.h>
#include <Ludens/UIF/UIFTheme.h>
#include <Ludens/UIF/UIFWindow.h>
#include <string>
#include <vector>

namespace LD {
namespace UIF {

enum WidgetType
{
    WIDGET_TYPE_WINDOW = 0,
    WIDGET_TYPE_BUTTON,
    WIDGET_TYPE_SLIDER,
    WIDGET_TYPE_TOGGLE,
    WIDGET_TYPE_PANEL,
    WIDGET_TYPE_IMAGE,
    WIDGET_TYPE_TEXT,
};

struct WidgetObj;
struct WindowObj;

struct ContextObj
{
    UIContext handle;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    PoolAllocator widgetPA;
    Theme theme;
    std::vector<WidgetObj*> windows;

    WidgetObj* alloc_widget(WidgetType type, WindowObj* window, void* user);
};

struct WindowObj
{
    ContextObj* ctx;
    UIWindow handle;
    WidgetNode node;
    std::string name;
    std::vector<Widget> children;
    Vec2 dragOffset;
    Vec2 dragBeginPos;
    Vec2 dragBeginSize;
    bool dragResize; // resize or reposition

    void update(float delta);

    static void on_drag(void* user, UIElement e, MouseButton btn, const Vec2& dragPos, bool begin);
};

struct ButtonWidgetObj
{
    WidgetObj* base;
    const char* text;
    void (*user_on_press)(ButtonWidget w, MouseButton btn, void* user);

    static void on_press(void* obj, UIElement handle, MouseButton btn);
    static void on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer);
};

struct SliderWidgetObj
{
    WidgetObj* base;
    Vec2 dragStart;
    float min;
    float max;
    float value;
    float ratio;

    static void on_drag(void* user, UIElement element, MouseButton btn, const Vec2& dragPos, bool begin);
    static void on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer);
};

struct ToggleWidgetObj
{
    WidgetObj* base;
    void (*user_on_toggle)(ToggleWidget w, bool state, void* user);
    Animation<QuadraticInterpolation> anim;
    bool state;

    static void on_press(void* obj, UIElement handle, MouseButton btn);
    static void on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer);
};

struct TextWidgetObj
{
    WidgetObj* base;
    const char* value;
    FontAtlas fontAtlas;
    float fontSize;

    static void wrap_limit_fn(void* user, float& outMinW, float& outMaxW);
    static float wrap_size_fn(void* user, float limitW);
    static void on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer);
};

struct PanelWidgetObj
{
    WidgetObj* base;
    uint32_t color;

    static void on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer);
};

struct ImageWidgetObj
{
    WidgetObj* base;
    RImage imageHandle;

    static void on_draw(WidgetObj* baesObj, ScreenRenderComponent renderer);
};

/// @brief A UIF Widget is a UIElement with well defined
///        user interaction via callbacks.
struct WidgetObj
{
    UIElement handle;
    WidgetNode node;
    WindowObj* window;
    void* user;
    Widget::DrawFn drawFn;
    WidgetType type;
    union
    {
        WindowObj window;
        TextWidgetObj text;
        PanelWidgetObj panel;
        ImageWidgetObj image;
        ButtonWidgetObj button;
        SliderWidgetObj slider;
        ToggleWidgetObj toggle;
    } as;
};

} // namespace UIF
} // namespace LD