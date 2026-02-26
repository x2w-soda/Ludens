#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/DSA/RectSplit.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/Text/TextBuffer.h>
#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>

#include <string>

// TODO:
#define UI_WORKSPACE_SPLIT_GAP 6.0f

namespace LD {

enum UIWidgetFlagBit
{
    /// @brief Widget subtree will not be drawn.
    UI_WIDGET_FLAG_HIDDEN_BIT = LD_BIT(0),

    /// @brief Widget handles the event on behalf of its subtree.
    UI_WIDGET_FLAG_BLOCK_EVENT_BIT = LD_BIT(1),

    /// @brief Widget subtree will be drawn with scissor.
    UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT = LD_BIT(2),

    /// @brief Widget can receive focus signals.
    UI_WIDGET_FLAG_FOCUSABLE_BIT = LD_BIT(3),

    /// @brief Widget 'handles' mouse events without an actual event handler function.
    UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT = LD_BIT(4),

    /// @brief Widget 'handles' key events without an actual event handler function.
    UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT = LD_BIT(5),

    /// @brief Widget 'handles' scroll events without an actual event handler function.
    UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT = LD_BIT(6),
};

struct UIWidgetObj;
struct UIWindowObj;
struct UIWorkspaceObj;
struct UILayerObj;
struct UIContextObj;

struct UILayout
{
    UILayoutInfo info;
    Rect rect;
    float minw;
    float minh;
};

struct UICallback
{
    bool (*onEvent)(UIWidget widget, const UIEvent& event);
    void (*onUpdate)(UIWidget widget, float delta);
    void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer);
};

/// @brief Docked UIWindow inside a UIWorkspace
struct UIWorkspaceNode
{
    Axis splitAxis;
    float splitRatio;
    bool isLeaf;
    UIAreaID nodeID;
    Rect rect;
    Rect splitRect;
    UIWorkspaceNode* parent = nullptr;
    UIWorkspaceNode* lch = nullptr;
    UIWorkspaceNode* rch = nullptr;
    UIWindow window = {};
};

/// @brief UI workspace implementation.
struct UIWorkspaceObj
{
    UILayerObj* layer = nullptr;
    HashSet<UIWindowObj*> deferredWindowDestruction;
    Vector<UIWindowObj*> nodeWindows;  // windows docked in workspace nodes
    Vector<UIWindowObj*> floatWindows; // floating windows
    RectSplit<UIWorkspaceNode, MEMORY_USAGE_UI> partition;
    const float splitGap = 6.0f; // TODO:
    uint32_t windowIDCounter = 0;
    uint32_t id = 0;       // workspace ID, unique within layer
    bool isHidden = false; // workspace level visibility mask

    UIWorkspaceObj() = delete;
    UIWorkspaceObj(const UIWorkspaceObj&) = delete;
    UIWorkspaceObj(const Rect& area)
        : partition(area, UI_WORKSPACE_SPLIT_GAP)
    {
    }
    ~UIWorkspaceObj();

    UIWorkspaceObj& operator=(const UIWorkspaceObj&) = delete;

    UIWindowObj* create_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);
    Hash64 get_hash() const;
    void pre_update();
    void update(float delta);
    void layout();
};

/// @brief UI layer implementation.
struct UILayerObj
{
    UIContextObj* ctx = nullptr;
    std::string name;
    HashSet<UIWorkspaceObj*> deferredWorkspaceDestruction;
    Vector<UIWorkspaceObj*> workspaces;
    uint32_t workspaceIDCounter = 0;

    UILayerObj() = default;
    UILayerObj(const UILayerObj&) = delete;
    ~UILayerObj();

    UILayerObj& operator=(const UILayerObj&) = delete;

    void pre_update();
    void update(float delta);
    void layout();
    void raise_workspace(UIWorkspaceObj* obj);
};

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
    void (*onEvent)(UIWidget, const UIEvent&, void*);
    Vec2 cursorPos;              /// mouse cursor global position
    Vec2 dragStartPos;           /// mouse cursor drag start global position
    MouseButton dragMouseButton; /// mouse button used for dragging

    UIWidgetObj* alloc_widget(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, void* user);
    void free_widget(UIWidgetObj* widget);

    UIWidgetObj* get_widget(const Vec2& pos);

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
    static bool on_event(UIWidget widget, const UIEvent& event);
};

struct UIButtonWidgetObj
{
    UIWidgetObj* base;
    const char* text;
    void (*onClick)(UIButtonWidget w, MouseButton btn, void* user);
    Color textColor;
    bool transparentBG;

    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

struct UISliderWidgetObj
{
    UIWidgetObj* base;
    Vec2 dragStart;
    float min;
    float max;
    float value;
    float ratio;

    static bool on_event(UIWidget widget, const UIEvent& event);
};

struct UIToggleWidgetObj
{
    UIWidgetObj* base;
    void (*user_on_toggle)(UIToggleWidget w, bool state, void* user);
    UIAnimation<QuadraticInterpolation> anim;
    bool state;

    static bool on_event(UIWidget widget, const UIEvent& event);
    static void on_update(UIWidget widget, float delta);
};

struct UITextWidgetObj
{
    UIWidgetObj* base;
    const char* value;
    FontAtlas fontAtlas;
    RImage fontImage;
    Color fgColor;
    Color bgColor;
    float fontSize;

    static void cleanup(UIWidgetObj* base);
};

struct UITextEditWidgetObj
{
    UIWidgetObj* base;
    TextBuffer<char> buf;
    UITextEditDomain domain;
    const char* placeHolder = nullptr;
    void (*onChange)(UITextEditWidget widget, View text, void* user);
    void (*onSubmit)(UITextEditWidget widget, View text, void* user);
    float fontSize;

    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidget widget, const UIEvent& event);

    void domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
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
    UILayout layout;               /// must be first field for layout semantics
    UICallback cb{};               /// callback function pointer table; TODO: single table for each type, not each instance
    UIWindowObj* window = nullptr; /// owning window
    UIWidgetObj* parent = nullptr; /// parent widget
    UIWidgetObj* child = nullptr;  /// first child widget
    UIWidgetObj* next = nullptr;   /// sibling widget
    UITheme theme{};               /// theme handle
    UINode node{};                 /// node in tree hierachy
    Vec2 scrollOffset{};           /// offset applied to children after layout
    std::string name;              /// widget debug name
    void* user = nullptr;          /// arbitrary user data
    const UIWidgetType type;       /// type enum
    uint32_t flags = 0;            /// widget bit flags
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
    } as{};

    UIWidgetObj() = delete;
    UIWidgetObj(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, UIWindowObj* window, void* user);
    UIWidgetObj(const UIWidgetObj&) = delete;
    ~UIWidgetObj();

    UIWidgetObj& operator=(const UIWidgetObj&) = delete;

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

    inline UIWidgetObj* get_child_by_name(const std::string& name)
    {
        for (UIWidgetObj* c = child; c; c = c->next)
        {
            if (c->name == name)
                return c;
        }

        return nullptr;
    }

    UIContextObj* ctx() const;

    /// @brief Draw the widget with default or custom render callback.
    void draw(ScreenRenderComponent renderer);
};

/// @brief UI Window implementation. A window is a specialized widget that
///        is directly managed by the UIContext.
struct UIWindowObj : UIWidgetObj
{
    UIWindowObj() = delete;
    UIWindowObj(const UILayoutInfo& layoutI);
    UIWindowObj(const UIWindowObj&) = delete;
    ~UIWindowObj();

    UIWindowObj& operator=(const UIWindowObj&) = delete;

    UIWorkspaceObj* space = nullptr; /// owning workspace
    Vector<UIWidgetObj*> widgets;    /// all widgets within the window
    Optional<Color> colorMask{};     /// optional mask to modify widget colors in window
    Color color = 0;                 /// window background color
    uint32_t id = 0;                 /// window ID, unique within workspace
    Vec2 dragOffset;
    Vec2 dragBeginPos;
    Vec2 dragBeginSize;
    void (*onResize)(UIWindow window, const Vec2& size) = nullptr;
    bool dragResize = false; // resize or reposition

    inline UIContextObj* ctx() const { return space->layer->ctx; }
    inline UILayerObj* layer() const { return space->layer; }
    Hash64 get_hash() const;
    void update(float delta);

    static void draw_widget_subtree(UIWidgetObj* widget, ScreenRenderComponent renderer);
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

/// @brief Perform UI layout on a widget subtree.
extern void ui_layout(UIWidgetObj* root);

/// @brief Perform any type specific cleanup or deallocations.
extern void ui_obj_cleanup(UIWidgetObj*);

} // namespace LD