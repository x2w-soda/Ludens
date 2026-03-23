#include <Ludens/DSA/Optional.h>
#include <Ludens/DSA/RectSplit.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/DocumentWindow/DocumentWindow.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/CreateComponentWindow/CreateComponentWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>
#include <LudensEditor/ProjectSettingsWindow/ProjectSettingsWindow.h>
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
    std::string uiLayerName;
    std::string uiWorkspaceName;
    Rect rootRect; // spans the entire EditorWorkspace
    RectSplit<EditorWorkspaceNode, MEMORY_USAGE_UI> partition;
    EditorWorkspaceControl control{};
    bool isVisible = true;
    bool isFloat = false;
    bool shouldClose = false;

    EditorWorkspaceObj() = delete;
    EditorWorkspaceObj(const EditorWorkspaceObj&) = delete;
    EditorWorkspaceObj(const Rect& area)
        : partition(area, EDITOR_WORKSPACE_SPLIT_GAP)
    {
    }
    ~EditorWorkspaceObj();

    EditorWorkspaceObj& operator=(const EditorWorkspaceObj&) = delete;

    bool get_hover_split_rect(Rect& outRect, Axis& outSplitAxis)
    {
        EditorWorkspaceNode* node = partition.get_node(control.dragSplitID);

        if (!node)
            node = partition.get_node(control.hoverSplitID);

        if (!node)
            return false;

        outRect = node->splitRect;
        outSplitAxis = node->splitAxis;
        return true;
    }

    void set_split_ratio(EditorAreaID areaID, float ratio);
    void set_rect(const Rect& rect);
    void set_pos(const Vec2& pos);
    void pre_imgui();
};

struct EditorWindowMeta
{
    EditorWindowType type;
    EditorIcon icon;
    EditorWindow (*create)(const EditorWindowInfo&);
    void (*destroy)(EditorWindow);
    const char* defaultTabName;
};

// clang-format off
static EditorWindowMeta sEditorWindowTable[] = {
    { EDITOR_WINDOW_TAB_CONTROL,      EDITOR_ICON_ENUM_LAST,        &TabControlWindow::create,      &TabControlWindow::destroy,      nullptr },
    { EDITOR_WINDOW_SELECTION,        EDITOR_ICON_ENUM_LAST,        &SelectionWindow::create,       &SelectionWindow::destroy,       "Selection" },
    { EDITOR_WINDOW_CREATE_COMPONENT, EDITOR_ICON_ENUM_LAST,        &CreateComponentWindow::create, &CreateComponentWindow::destroy, "CreateComponent" },
    { EDITOR_WINDOW_PROJECT_SETTINGS, EDITOR_ICON_ENUM_LAST,        &ProjectSettingsWindow::create, &ProjectSettingsWindow::destroy, "ProjectSettings" },
    { EDITOR_WINDOW_DOCUMENT,         EDITOR_ICON_ENUM_LAST,        &DocumentWindow::create,        &DocumentWindow::destroy,        "Document" },
    { EDITOR_WINDOW_VIEWPORT,         EDITOR_ICON_VIEWPORT_WINDOW,  &ViewportWindow::create,        &ViewportWindow::destroy,        "Viewport" },
    { EDITOR_WINDOW_OUTLINER,         EDITOR_ICON_OUTLINER_WINDOW,  &OutlinerWindow::create,        &OutlinerWindow::destroy,        "Outliner" },
    { EDITOR_WINDOW_INSPECTOR,        EDITOR_ICON_INSPECTOR_WINDOW, &InspectorWindow::create,       &InspectorWindow::destroy,       "Inspector" },
    { EDITOR_WINDOW_CONSOLE,          EDITOR_ICON_CONSOLE_WINDOW,   &ConsoleWindow::create,         &ConsoleWindow::destroy,         "Console" },
    { EDITOR_WINDOW_VERSION,          EDITOR_ICON_ENUM_LAST,        &VersionWindow::create,         &VersionWindow::destroy,         "Version" },
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
        node->tabControl.set_rect(tabControlRect);
        node->window.set_rect(windowRect);
    });
}

void EditorWorkspaceObj::set_rect(const Rect& rect)
{
    partition.set_root_rect(rect);
    rootRect = rect;

    partition.visit_leaves(partition.get_root_id(), [](EditorWorkspaceNode* node) {
        Rect tabControlRect, windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControl.set_rect(tabControlRect);
        node->window.set_rect(windowRect);
    });
}

void EditorWorkspaceObj::set_pos(const Vec2& pos)
{
    partition.set_root_pos(pos);
    rootRect.set_pos(pos.x, pos.y);

    partition.visit_leaves(partition.get_root_id(), [](EditorWorkspaceNode* node) {
        Rect tabControlRect, windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControl.set_pos(tabControlRect.get_pos());
        node->window.set_pos(windowRect.get_pos());
    });
}

void EditorWorkspaceObj::pre_imgui()
{
    if (shouldClose)
        return;

    Vector<EditorAreaID> toClose;
    EditorAreaID rootID = partition.get_root_id();

    partition.visit_leaves(rootID, [&](EditorWorkspaceNode* node) {
        if (node->window.should_close())
            toClose.push_back(node->nodeID);
    });

    if (!toClose.empty())
    {
        LD_ASSERT(toClose.size() == 1 && toClose[0] == rootID);
        shouldClose = true;
    }
}

//
// Public API
//

EditorWorkspace EditorWorkspace::create(const EditorWorkspaceInfo& spaceI)
{
    auto* obj = heap_new<EditorWorkspaceObj>(MEMORY_USAGE_UI, spaceI.rootRect);
    obj->ctx = spaceI.ctx;
    obj->uiLayerName = spaceI.uiLayerName;
    obj->uiWorkspaceName = spaceI.uiWorkspaceName;
    obj->isFloat = spaceI.isFloat;
    obj->isVisible = spaceI.isVisible;
    obj->rootRect = spaceI.rootRect;

    LD_ASSERT(spaceI.isVisible);
    EditorWorkspace space = EditorWorkspace(obj);
    // space.set_visible(spaceI.isVisible);

    return space;
}

void EditorWorkspace::destroy(EditorWorkspace space)
{
    auto* obj = space.unwrap();

    heap_delete<EditorWorkspaceObj>(obj);
}

bool EditorWorkspace::should_close()
{
    return mObj->shouldClose;
}

void EditorWorkspace::set_visible(bool isVisible)
{
    LD_UNREACHABLE;

    mObj->isVisible = isVisible;

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
        if (node->tabControl)
            ; // node->tabControl.set_visible(isVisible);

        if (node->window)
            ; // node->window.set_visible(isVisible);
    });
}

EditorWindow EditorWorkspace::create_window(EditorAreaID areaID, EditorWindowType type)
{
    EditorWorkspaceNode* node = mObj->partition.get_node(areaID);

    if (node->window)
    {
        destroy_window(node->window);
    }

    const char* tabName = sEditorWindowTable[(int)type].defaultTabName;
    EditorIcon tabIcon = sEditorWindowTable[(int)type].icon;
    std::string tabWorkspaceName(tabName);
    tabWorkspaceName += "Tab";
    EditorWindowInfo windowI{};
    windowI.ctx = mObj->ctx;
    windowI.uiWorkspaceName = tabWorkspaceName.c_str();
    node->tabControl = sEditorWindowTable[(int)EDITOR_WINDOW_TAB_CONTROL].create(windowI);

    TabControlWindow tabControl = (TabControlWindow)node->tabControl;
    tabControl.set_window_type(type, tabName, tabIcon);

    windowI.uiWorkspaceName = tabName;
    return node->window = sEditorWindowTable[(int)type].create(windowI);
}

void EditorWorkspace::destroy_window(EditorWindow window)
{
    sEditorWindowTable[(int)window.get_type()].destroy(window);
}

void EditorWorkspace::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    mObj->pre_imgui();

    if (mObj->shouldClose)
        return;

    // each EditorWindow contains a UIWorkspaces, but they all belong to the same UILayer.
    ui_layer_begin(mObj->uiLayerName.c_str());

    // EditorWorkspace root window detects resizing.
    ui_workspace_begin(mObj->uiWorkspaceName.c_str(), mObj->rootRect);
    ui_push_window("WORKSPACE_ROOT");
    ui_top_user(mObj);

    // If EditorWindow does not catch key events, capture here and forward to EditorContext.
    KeyValue keyVal;
    if (ui_top_key_down(keyVal))
        mObj->ctx.input_key_value(keyVal);

    // find the node for splitting.
    UIEventType type;
    Vec2 mousePos;
    if (ui_top_hover(type))
    {
        if (type == UI_EVENT_MOUSE_ENTER && ui_top_get_mouse_pos(mousePos))
        {
            Rect rect;
            ui_top_get_rect(rect);
            const Vec2 screenPos = mousePos + rect.get_pos();

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
        else if (type == UI_EVENT_MOUSE_LEAVE)
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

    if (mObj->control.dragSplitID && !ui_top_is_dragged())
    {
        mObj->control.dragSplitID = 0;
    }

    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (EditorWorkspaceObj*)user;
        EditorTheme theme = obj->ctx.get_theme();

        Axis splitAxis;
        Rect splitRect;
        if (obj->get_hover_split_rect(splitRect, splitAxis))
        {
            if (splitAxis == AXIS_X)
            {
                splitRect = Rect::scale_w(splitRect, 0.8f);
                splitRect = Rect::scale_h(splitRect, 0.5f);
            }
            else
            {
                splitRect = Rect::scale_h(splitRect, 0.8f);
                splitRect = Rect::scale_w(splitRect, 0.5f);
            }

            Color color = theme.get_ui_theme().get_background_color();
            renderer.draw_rect(splitRect, Color::lift(color, 0.18f));
        }
    });

    ui_pop_window();
    ui_workspace_end();

    Optional<Vec2> newWorkspacePos;

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
        Rect tabControlRect;
        Rect windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControl.set_rect(tabControlRect);
        node->tabControl.on_imgui(delta);
        node->window.set_rect(windowRect);
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

    ui_layer_end();
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
