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

#include "UIContextObj.h"
#include "UIWidgetObj.h"

// TODO:
#define UI_WORKSPACE_SPLIT_GAP 6.0f

namespace LD {

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
    bool isVisible = true; // workspace level visibility mask

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
    void update_windows(float delta);
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
    bool isVisible = true;

    UILayerObj() = default;
    UILayerObj(const UILayerObj&) = delete;
    ~UILayerObj();

    UILayerObj& operator=(const UILayerObj&) = delete;

    void pre_update();
    void update(float delta);
    void layout();
    void raise_workspace(UIWorkspaceObj* obj);
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

    // updates all widgets within window
    void update_widgets(float delta);

    static void draw_widget_subtree(UIWidgetObj* widget, ScreenRenderComponent renderer);
    static void on_draw(UIWidgetObj* widget, ScreenRenderComponent renderer);
    static bool on_event(UIWidgetObj* widget, const UIEvent& event);
};

/// @brief Perform UI layout on a widget subtree.
extern void ui_layout(UIWidgetObj* root);

} // namespace LD