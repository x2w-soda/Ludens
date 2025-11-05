#pragma once

#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace LD {

enum UIWidgetFlagBit
{
    /// @brief Widget subtree will not be drawn.
    UI_WIDGET_FLAG_HIDDEN_BIT = LD_BIT(0),

    /// @brief Widget will not respond to mouse and key input,
    ///        consuming the input event without propagating.
    ///        This is usually set during short animations.
    UI_WIDGET_FLAG_BLOCK_INPUT_BIT = LD_BIT(1),

    /// @brief Widget subtree will be drawn with scissor.
    UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT = LD_BIT(2),
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
    void (*onKey)(UIWidget widget, KeyCode key, UIEvent event);
    void (*onMouse)(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    void (*onDrag)(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    void (*onHover)(UIWidget widget, UIEvent event);
    void (*onScroll)(UIWidget widget, const Vec2& offset);
};

struct UIContextLayer
{
    Hash32 layerHash;
    std::vector<UIWindowObj*> windows;
};

struct UIContextObj
{
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    PoolAllocator widgetPA;
    UITheme theme;
    std::vector<UIContextLayer*> layers;
    std::unordered_set<UIWindowObj*> deferredWindowDestruction;
    UIWidgetObj* dragWidget;     /// the widget begin dragged
    UIWidgetObj* pressWidget;    /// the widget pressed and not yet released
    UIWidgetObj* cursorWidget;   /// the widget under mouse cursor
    Vec2 cursorPos;              /// mouse cursor global position
    Vec2 dragStartPos;           /// mouse cursor drag start global position
    MouseButton dragMouseButton; /// mouse button used for dragging

    UIWidgetObj* alloc_widget(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, void* user);
    void free_widget(UIWidgetObj* widget);

    UIWidgetObj* get_widget(const Vec2& pos, int filter);

    void pre_update(float delta);

    UIContextLayer* get_or_create_layer(Hash32 layerHash);
    UIContextLayer* get_layer(Hash32 layerHash);
    void remove_layer(UIContextLayer* toRemove);

    /// @brief Raise a window to top within its layer.
    /// @param window Target window.
    void raise_window(UIWindowObj* window);

    /// @brief When a widget is removed, reset all references to it.
    void invalidate_refs(UIWidgetObj* removed);
};

/// @brief Scroll widget implementation.
struct UIScrollWidgetObj
{
    UIWidgetObj* base;
    float offsetXDst;   // destination value for scrollOffset x
    float offsetYDst;   // destination value for scrollOffset y
    float offsetXSpeed; // animation speed for scrollOffset x
    float offsetYSpeed; // animation speed for scrollOffset y
    Color bgColor;
    bool hasScrollBar;

    static void cleanup(UIWidgetObj* base);
    static void on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_scroll(UIWidget widget, const Vec2& offset);
};

struct UIButtonWidgetObj
{
    UIWidgetObj* base;
    const char* text;
    void (*user_on_press)(UIButtonWidget w, MouseButton btn, void* user);
    Color textColor;
    bool transparentBG;

    static void cleanup(UIWidgetObj* base);
    static void on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_hover(UIWidget widget, UIEvent event);
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
};

struct UIToggleWidgetObj
{
    UIWidgetObj* base;
    void (*user_on_toggle)(UIToggleWidget w, bool state, void* user);
    UIAnimation<QuadraticInterpolation> anim;
    bool state;

    static void on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_update(UIWidget widget, float delta);
};

struct UITextWidgetObj
{
    UIWidgetObj* base;
    const char* value;
    FontAtlas fontAtlas;
    Color bgColor;
    float fontSize;
    bool hoverHL;

    static void cleanup(UIWidgetObj* base);
};

struct UITextEditWidgetObj
{
    UIWidgetObj* base;
    std::string* value;
    const char* placeHolder;
    float fontSize;

    static void cleanup(UIWidgetObj* base);
    static void on_key(UIWidget widget, KeyCode key, UIEvent event);
};

struct UIPanelWidgetObj
{
    UIWidgetObj* base;
    Color color;
};

struct UIImageWidgetObj
{
    UIWidgetObj* base;
    RImage imageHandle;
    Rect imageRect;
    Color tint;
};

/// @brief UI Widget implementation
struct UIWidgetObj
{
    UILayout layout;     /// must be first field for layout semantics
    UICallback cb;       /// callback function pointer table; TODO: single table for each type, not each instance
    UIWindowObj* window; /// owning window
    UIWidgetObj* parent; /// parent widget
    UIWidgetObj* child;  /// first child widget
    UIWidgetObj* next;   /// sibling widget
    UITheme theme;       /// theme handle
    UINode node;         /// node in tree hierachy
    Vec2 scrollOffset;   /// offset applied to children after layout
    void* user;          /// arbitrary user data
    UIWidgetType type;   /// type enum
    uint32_t flags;      /// widget bit flags
    union
    {
        UIScrollWidgetObj scroll;
        UITextWidgetObj text;
        UITextEditWidgetObj textEdit;
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

    /// @brief Draw the widget with default or custom render callback.
    void draw(ScreenRenderComponent renderer);
};

/// @brief UI Window implementation. A window is a specialized widget that
///        is directly managed by the UIContext.
struct UIWindowObj : UIWidgetObj
{
    UIWindowObj();
    UIWindowObj(const UIWindowObj&) = delete;
    ~UIWindowObj();

    UIWindowObj& operator=(const UIWindowObj&) = delete;

    UIContextObj* ctx;                 /// owning context
    UIContextLayer* layer;             /// residing layer
    std::string name;                  /// window identifier
    std::vector<UIWidgetObj*> widgets; /// all widgets within the window
    std::optional<Color> colorMask;    /// optional mask to modify widget colors in window
    Vec2 dragOffset;
    Vec2 dragBeginPos;
    Vec2 dragBeginSize;
    bool dragResize; // resize or reposition

    void update(float delta);

    static void draw_widget_subtree(UIWidgetObj* widget, ScreenRenderComponent renderer);

    static void on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
};

/// @brief Perform UI layout on a widget subtree.
extern void ui_layout(UIWidgetObj* root);

/// @brief Perform any type specific cleanup or deallocations.
extern void ui_obj_cleanup(UIWidgetObj*);

} // namespace LD