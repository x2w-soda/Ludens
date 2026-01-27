#include <Ludens/DSA/Optional.h>
#include <Ludens/DSA/RectSplit.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>
#include <LudensEditor/TabControlWindow/TabControlWindow.h>
#include <LudensEditor/VersionWindow/VersionWindow.h>
#include <LudensEditor/ViewportWindow/ViewportWindow.h>

#define EDITOR_TAB_HEIGHT 20.0f
#define EDITOR_WORKSPACE_SPLIT_GAP 6.0f

namespace LD {

struct EditorWorkspaceNode
{
    EditorWorkspaceNode* parent;
    EditorWorkspaceNode* lch;
    EditorWorkspaceNode* rch;
    EditorAreaID nodeID;
    EditorWindow window;
    EditorWindow tabControl;
    UIWorkspace windowWorkspace;
    UIWorkspace tabControlWorkspace;
    Rect splitRect;
    Axis splitAxis;
    float splitRatio;
    bool isLeaf;
    Rect rect;

    void get_workspace_rects(Rect& tabControlRect, Rect& windowRect)
    {
        tabControlRect = rect;
        tabControlRect.h = EDITOR_TAB_HEIGHT;
        windowRect = rect;
        windowRect.y += EDITOR_TAB_HEIGHT;
        windowRect.h = rect.h - EDITOR_TAB_HEIGHT;
    }
};

/// @brief Editor workspace resizing, dragging, and partitioning controls.
struct EditorWorkspaceControl
{
    Vec2 dragOffset;
    Axis hoverSplitAxis;
    Axis dragSplitAxis;
    EditorAreaID hoverSplitID;
    EditorAreaID dragSplitID;
};

struct EditorWorkspaceObj
{
    EditorContext ctx;
    UILayer layer;
    UIWorkspace rootWS;
    RectSplit<EditorWorkspaceNode, MEMORY_USAGE_UI> partition;
    EditorWorkspaceControl control{};
    bool isVisible = true;
    bool isFloat = false;

    EditorWorkspaceObj() = delete;
    EditorWorkspaceObj(const EditorWorkspaceObj&) = delete;
    EditorWorkspaceObj(const Rect& area)
        : partition(area, EDITOR_WORKSPACE_SPLIT_GAP)
    {
    }
    ~EditorWorkspaceObj();

    UIWorkspaceObj& operator=(const EditorWorkspaceObj&) = delete;

    bool get_hover_split_rect(Rect& outRect, Axis& outSplitAxis)
    {
        EditorWorkspaceNode* node = partition.get_node(control.hoverSplitID);
        if (!node)
            return false;

        outRect = node->splitRect;
        outSplitAxis = node->splitAxis;
        return true;
    }

    void set_split_ratio(EditorAreaID areaID, float ratio);
    void set_rect(const Rect& rect);
    void set_pos(const Vec2& pos);
};

struct EditorWindowMeta
{
    EditorWindowType type;
    EditorWindow (*create)(const EditorWindowInfo&);
    void (*destroy)(EditorWindow);
    const char* defaultTabName;
};

// clang-format off
static EditorWindowMeta sEditorWindowTable[] = {
    { EDITOR_WINDOW_TAB_CONTROL, &TabControlWindow::create, &TabControlWindow::destroy, nullptr },
    { EDITOR_WINDOW_SELECTION,   &SelectionWindow::create,  &SelectionWindow::destroy, "Selection" },
    { EDITOR_WINDOW_VIEWPORT,    &ViewportWindow::create,   &ViewportWindow::destroy,  "Viewport" },
    { EDITOR_WINDOW_OUTLINER,    &OutlinerWindow::create,   &OutlinerWindow::destroy,  "Outliner" },
    { EDITOR_WINDOW_INSPECTOR,   &InspectorWindow::create,  &InspectorWindow::destroy, "Inspector" },
    { EDITOR_WINDOW_CONSOLE,     &ConsoleWindow::create,    &ConsoleWindow::destroy,   "Console" },
    { EDITOR_WINDOW_VERSION,     &VersionWindow::create,    &VersionWindow::destroy,   "Version" },
};
// clang-format on

static_assert(sizeof(sEditorWindowTable) / sizeof(*sEditorWindowTable) == EDITOR_WINDOW_TYPE_ENUM_COUNT);

EditorWorkspaceObj::~EditorWorkspaceObj()
{
    partition.visit_leaves(partition.get_root_id(), [](EditorWorkspaceNode* node) {
        sEditorWindowTable[(int)node->window.get_type()].destroy(node->window);
        sEditorWindowTable[EDITOR_WINDOW_TAB_CONTROL].destroy(node->tabControl);
    });
}

void EditorWorkspaceObj::set_split_ratio(EditorAreaID areaID, float ratio)
{
    partition.set_split_ratio(areaID, ratio);

    partition.visit_leaves(areaID, [](EditorWorkspaceNode* node) {
        Rect tabControlRect, windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControlWorkspace.set_rect(tabControlRect);
        node->windowWorkspace.set_rect(windowRect);
    });
}

void EditorWorkspaceObj::set_rect(const Rect& rect)
{
    partition.set_root_rect(rect);
    rootWS.set_rect(rect);

    partition.visit_leaves(partition.get_root_id(), [](EditorWorkspaceNode* node) {
        Rect tabControlRect, windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControlWorkspace.set_rect(tabControlRect);
        node->windowWorkspace.set_rect(windowRect);
    });
}

void EditorWorkspaceObj::set_pos(const Vec2& pos)
{
    partition.set_root_pos(pos);
    rootWS.set_pos(pos);

    partition.visit_leaves(partition.get_root_id(), [](EditorWorkspaceNode* node) {
        Rect tabControlRect, windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControlWorkspace.set_pos(tabControlRect.get_pos());
        node->windowWorkspace.set_pos(windowRect.get_pos());
    });
}

//
// Public API
//

EditorWorkspace EditorWorkspace::create(const EditorWorkspaceInfo& spaceI)
{
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(spaceI.rootRect.x);
    layoutI.sizeY = UISize::fixed(spaceI.rootRect.y);

    auto* obj = heap_new<EditorWorkspaceObj>(MEMORY_USAGE_UI, spaceI.rootRect);
    obj->ctx = spaceI.ctx;
    obj->layer = spaceI.layer;
    obj->isFloat = spaceI.isFloat;
    obj->isVisible = spaceI.isVisible;
    obj->rootWS = obj->layer.create_workspace(spaceI.rootRect);
    obj->rootWS.create_window(obj->rootWS.get_root_id(), layoutI, {}, nullptr);

    EditorWorkspace space = EditorWorkspace(obj);
    space.set_visible(spaceI.isVisible);

    return space;
}

void EditorWorkspace::destroy(EditorWorkspace space)
{
    auto* obj = space.unwrap();

    obj->layer.destroy_workspace(obj->rootWS);

    heap_delete<EditorWorkspaceObj>(obj);
}

void EditorWorkspace::set_visible(bool isVisible)
{
    mObj->isVisible = isVisible;
    mObj->rootWS.set_visible(isVisible);

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
        if (node->tabControlWorkspace)
            node->tabControlWorkspace.set_visible(isVisible);

        if (node->windowWorkspace)
            node->windowWorkspace.set_visible(isVisible);
    });
}

EditorWindow EditorWorkspace::create_window(EditorAreaID areaID, EditorWindowType type)
{
    EditorWorkspaceNode* node = mObj->partition.get_node(areaID);

    if (node->window)
    {
        destroy_window(node->window);
        mObj->layer.destroy_workspace(node->windowWorkspace);
        mObj->layer.destroy_workspace(node->tabControlWorkspace);
    }

    Rect tabRect, windowRect;
    node->get_workspace_rects(tabRect, windowRect);
    node->tabControlWorkspace = mObj->layer.create_workspace(tabRect);
    node->tabControlWorkspace.set_visible(mObj->isVisible);
    node->windowWorkspace = mObj->layer.create_workspace(windowRect);
    node->windowWorkspace.set_visible(mObj->isVisible);

    EditorWindowInfo windowI{};
    windowI.ctx = mObj->ctx;
    windowI.space = node->tabControlWorkspace;
    node->tabControl = sEditorWindowTable[(int)EDITOR_WINDOW_TAB_CONTROL].create(windowI);
    static_cast<TabControlWindow>(node->tabControl).set_tab_name(sEditorWindowTable[(int)type].defaultTabName);

    windowI.space = node->windowWorkspace;
    node->window = sEditorWindowTable[(int)type].create(windowI);

    return node->window;
}

void EditorWorkspace::destroy_window(EditorWindow window)
{
    sEditorWindowTable[(int)window.get_type()].destroy(window);
}

void EditorWorkspace::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    Optional<Vec2> newWorkspacePos;

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
        node->tabControl.on_imgui(delta);
        node->window.on_imgui(delta);

        bool beginDrag;
        Vec2 screenPos;
        MouseButton btn;
        TabControlWindow tabControlW = (TabControlWindow)node->tabControl;
        if (mObj->isFloat && tabControlW.has_drag(btn, screenPos, beginDrag) && btn == MOUSE_BUTTON_LEFT)
        {
            if (beginDrag)
                mObj->control.dragOffset = node->rect.get_pos() - screenPos;
            else
                newWorkspacePos = screenPos + mObj->control.dragOffset;
        }
    });

    // reposition editor workspace
    if (newWorkspacePos.has_value())
    {
        mObj->set_pos(newWorkspacePos.value());
    }

    // root window spans the entire editor workspace and detects resizing.
    UIWindow rootW = mObj->rootWS.get_area_window(mObj->rootWS.get_root_id());
    ui_push_window(rootW);
    ui_top_user(mObj);

    // find the node for splitting.
    UIEvent event;
    Vec2 mousePos;
    if (ui_top_hover(event))
    {
        if (event == UI_MOUSE_ENTER && rootW.get_mouse_pos(mousePos))
        {
            const Vec2 screenPos = mousePos + rootW.get_pos();

            mObj->partition.visit_nodes(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
                if (node->isLeaf)
                    return;

                if (node->rect.contains(screenPos))
                {
                    mObj->control.hoverSplitID = node->nodeID;
                    mObj->control.hoverSplitAxis = node->splitAxis;
                }
            });
        }
        else if (event == UI_MOUSE_LEAVE)
        {
            mObj->control.hoverSplitID = 0;
        }
    }

    bool begin;
    MouseButton btn;
    if (ui_top_drag(btn, mousePos, begin) && btn == MOUSE_BUTTON_LEFT)
    {
        if (begin)
        {
            mObj->control.dragSplitID = mObj->control.hoverSplitID;
            mObj->control.dragSplitAxis = mObj->control.hoverSplitAxis;
        }

        EditorWorkspaceNode* node = mObj->partition.get_node(mObj->control.dragSplitID);

        if (node)
        {
            float ratio = 0.0f;
            if ((int)node->splitAxis == (int)UI_AXIS_X)
                ratio = (mousePos.y - node->rect.y) / node->rect.h;
            else
                ratio = (mousePos.x - node->rect.x) / node->rect.w;

            mObj->set_split_ratio(node->nodeID, ratio);
        }
    }

    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (EditorWorkspaceObj*)user;
        EditorTheme theme = obj->ctx.get_theme();

        Axis splitAxis;
        Rect splitRect;
        if (obj->get_hover_split_rect(splitRect, splitAxis))
        {
            if (splitAxis == AXIS_X)
                splitRect = Rect::scale_w(splitRect, 0.5f);
            else
                splitRect = Rect::scale_h(splitRect, 0.5f);

            renderer.draw_rect(splitRect, theme.get_ui_theme().get_field_color());
        }
    });

    ui_pop_window();
}

void EditorWorkspace::set_rect(const Rect& rect)
{
    mObj->set_rect(rect);
}

EditorAreaID EditorWorkspace::get_root_id()
{
    return mObj->partition.get_root_id();
}

EditorAreaID EditorWorkspace::split_right(EditorAreaID areaID, float ratio)
{
    return mObj->partition.split_right(areaID, ratio);
}

EditorAreaID EditorWorkspace::split_bottom(EditorAreaID areaID, float ratio)
{
    return mObj->partition.split_bottom(areaID, ratio);
}

} // namespace LD
