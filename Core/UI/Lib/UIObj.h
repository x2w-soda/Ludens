#pragma once

#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <string>
#include <vector>

namespace LD {

enum UIWidgetType
{
    UI_WIDGET_WINDOW = 0,
    UI_WIDGET_BUTTON,
    UI_WIDGET_SLIDER,
    UI_WIDGET_TOGGLE,
    UI_WIDGET_PANEL,
    UI_WIDGET_IMAGE,
    UI_WIDGET_TEXT,
};

struct UIWidgetObj;
struct UIWindowObj;

struct UILayout
{
    UILayoutInfo info;
    Rect rect;
    float minw;
    float minh;
};

struct UICallback
{
    void (*onUpdate)(UIWidget widget, float delta);
    void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer);
    void (*onKeyUp)(UIWidget widget, KeyCode key);
    void (*onKeyDown)(UIWidget widget, KeyCode key);
    void (*onMouseUp)(UIWidget widget, const Vec2& pos, MouseButton btn);
    void (*onMouseDown)(UIWidget widget, const Vec2& pos, MouseButton btn);
    void (*onDrag)(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    void (*onEnter)(UIWidget widget);
    void (*onLeave)(UIWidget widget);
};

struct UIContextObj
{
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    PoolAllocator widgetPA;
    UITheme theme;
    std::vector<UIWindowObj*> windows;
    UIWidgetObj* dragElement;    /// the widget begin dragged
    UIWidgetObj* pressElement;   /// the widget pressed and not yet released
    UIWidgetObj* cursorElement;  /// the widget under mouse cursor
    Vec2 cursorPos;              /// mouse cursor position
    Vec2 dragStartPos;           /// mouse cursor drag start position
    MouseButton dragMouseButton; /// mouse button used for dragging

    UIWidgetObj* alloc_widget(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, void* user);
};

struct UIButtonWidgetObj
{
    UIWidgetObj* base;
    const char* text;
    void (*user_on_press)(UIButtonWidget w, MouseButton btn, void* user);

    static void on_press(UIWidget widget, const Vec2& pos, MouseButton btn);
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UISliderWidgetObj
{
    UIWidgetObj* base;
    Vec2 dragStart;
    float min;
    float max;
    float value;
    float ratio;

    static void on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIToggleWidgetObj
{
    UIWidgetObj* base;
    void (*user_on_toggle)(UIToggleWidget w, bool state, void* user);
    UIAnimation<QuadraticInterpolation> anim;
    bool state;

    static void on_press(UIWidget widget, const Vec2& pos, MouseButton btn);
    static void on_update(UIWidget widget, float delta);
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UITextWidgetObj
{
    UIWidgetObj* base;
    const char* value;
    FontAtlas fontAtlas;
    float fontSize;
    bool hoverHL;

    static void wrap_limit_fn(UIWidgetObj* widget, float& outMinW, float& outMaxW);
    static float wrap_size_fn(UIWidgetObj* widget, float limitW);
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIPanelWidgetObj
{
    UIWidgetObj* base;
    uint32_t color;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIImageWidgetObj
{
    UIWidgetObj* base;
    RImage imageHandle;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

/// @brief UI Widget implementation
struct UIWidgetObj
{
    UILayout layout;     /// must be first field for layout semantics
    UICallback cb;       /// callback function pointer table
    UIWindowObj* window; /// owning window
    UIWidgetObj* parent; /// parent widget
    UIWidgetObj* child;  /// first child widget
    UIWidgetObj* next;   /// sibling widget
    UINode node;         /// node in tree hierachy
    void* user;          /// arbitrary user data
    UIWidgetType type;   /// type enum
    union
    {
        UITextWidgetObj text;
        UIPanelWidgetObj panel;
        UIImageWidgetObj image;
        UIButtonWidgetObj button;
        UISliderWidgetObj slider;
        UIToggleWidgetObj toggle;
    } as;

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

    /// @brief get children count in linear time
    inline int get_children_count()
    {
        int count = 0;
        for (UIWidgetObj* c = child; c; c = c->next)
            count++;
        return count;
    }
};

/// @brief UI Window implementation. A window is a specialized widget that
///        is directly managed by the UIContext.
struct UIWindowObj : UIWidgetObj
{
    UIWindowObj() : UIWidgetObj{} {}

    UIContextObj* ctx;                 /// owning context
    std::string name;                  /// window identifier
    std::vector<UIWidgetObj*> widgets; /// all widgets within the window
    Vec2 dragOffset;
    Vec2 dragBeginPos;
    Vec2 dragBeginSize;
    bool dragResize; // resize or reposition
    bool isHidden;

    void update(float delta);

    static void on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
};

extern void ui_layout(UIWidgetObj* root);

} // namespace LD