#include <Ludens/DSA/Optional.h>
#include <Ludens/DSA/RectSplit.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/AssetImportWindow/AssetImportWindow.h>
#include <LudensEditor/AssetSelectWindow/AssetSelectWindow.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/CreateComponentWindow/CreateComponentWindow.h>
#include <LudensEditor/DocumentWindow/DocumentWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>
#include <LudensEditor/ProjectSettingsWindow/ProjectSettingsWindow.h>
#include <LudensEditor/ProjectWindow/ProjectWindow.h>
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
    std::string uiContextName;
    std::string uiLayerName;
    Color rootColor = 0;
    Rect rootRect = {}; // spans the entire EditorWorkspace
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
};

struct EditorWindowMeta
{
    EditorWindowType type;
    EditorIcon icon;
    EditorWindow (*create)(const EditorWindowInfo&);
    void (*destroy)(EditorWindow);
    void (*preUpdate)(EditorWindowObj*, const EditorUpdateTick& tick);
    void (*update)(EditorWindowObj*, const EditorUpdateTick& tick);
    const char* defaultName;
};

// clang-format off
static EditorWindowMeta sEditorWindow[] = {
    { EDITOR_WINDOW_TAB_CONTROL,      EDITOR_ICON_ENUM_LAST,        &TabControlWindow::create,      &TabControlWindow::destroy,      nullptr,                     &TabControlWindow::update,      nullptr },
    { EDITOR_WINDOW_ASSET_IMPORT,     EDITOR_ICON_ENUM_LAST,        &AssetImportWindow::create,     &AssetImportWindow::destroy,     nullptr,                     &AssetImportWindow::update,     "AssetImport" },
    { EDITOR_WINDOW_ASSET_SELECT,     EDITOR_ICON_ENUM_LAST,        &AssetSelectWindow::create,     &AssetSelectWindow::destroy,     nullptr,                     &AssetSelectWindow::update,     "AssetSelect" },
    { EDITOR_WINDOW_SELECTION,        EDITOR_ICON_ENUM_LAST,        &SelectionWindow::create,       &SelectionWindow::destroy,       nullptr,                     &SelectionWindow::update,       "Selection" },
    { EDITOR_WINDOW_CREATE_COMPONENT, EDITOR_ICON_ENUM_LAST,        &CreateComponentWindow::create, &CreateComponentWindow::destroy, nullptr,                     &CreateComponentWindow::update, "CreateComponent" },
    { EDITOR_WINDOW_PROJECT_SETTINGS, EDITOR_ICON_ENUM_LAST,        &ProjectSettingsWindow::create, &ProjectSettingsWindow::destroy, nullptr,                     &ProjectSettingsWindow::update, "ProjectSettings" },
    { EDITOR_WINDOW_PROJECT,          EDITOR_ICON_ENUM_LAST,        &ProjectWindow::create,         &ProjectWindow::destroy,         nullptr,                     &ProjectWindow::update,         "Project" },
    { EDITOR_WINDOW_DOCUMENT,         EDITOR_ICON_ENUM_LAST,        &DocumentWindow::create,        &DocumentWindow::destroy,        &DocumentWindow::pre_update, &DocumentWindow::update,        "Document" },
    { EDITOR_WINDOW_VIEWPORT,         EDITOR_ICON_VIEWPORT_WINDOW,  &ViewportWindow::create,        &ViewportWindow::destroy,        nullptr,                     &ViewportWindow::update,        "Viewport" },
    { EDITOR_WINDOW_OUTLINER,         EDITOR_ICON_OUTLINER_WINDOW,  &OutlinerWindow::create,        &OutlinerWindow::destroy,        nullptr,                     &OutlinerWindow::update,        "Outliner" },
    { EDITOR_WINDOW_INSPECTOR,        EDITOR_ICON_INSPECTOR_WINDOW, &InspectorWindow::create,       &InspectorWindow::destroy,       nullptr,                     &InspectorWindow::update,       "Inspector" },
    { EDITOR_WINDOW_CONSOLE,          EDITOR_ICON_CONSOLE_WINDOW,   &ConsoleWindow::create,         &ConsoleWindow::destroy,         nullptr,                     &ConsoleWindow::update,         "Console" },
    { EDITOR_WINDOW_VERSION,          EDITOR_ICON_ENUM_LAST,        &VersionWindow::create,         &VersionWindow::destroy,         nullptr,                     &VersionWindow::update,         "Version" },
};
// clang-format on

static_assert(sizeof(sEditorWindow) / sizeof(*sEditorWindow) == EDITOR_WINDOW_TYPE_ENUM_COUNT);

static EditorWindow editor_window_create(const EditorWindowInfo& info)
{
    return sEditorWindow[(int)info.type].create(info);
}

static void editor_window_destroy(EditorWindow window)
{
    sEditorWindow[(int)window.type()].destroy(window);
}

static void editor_window_pre_update(EditorWindowObj* obj, const EditorUpdateTick& tick)
{
    if (sEditorWindow[(int)obj->type].preUpdate)
        sEditorWindow[(int)obj->type].preUpdate(obj, tick);
}

static void editor_window_update(EditorWindowObj* obj, const EditorUpdateTick& tick)
{
    sEditorWindow[(int)obj->type].update(obj, tick);
}

EditorWorkspaceObj::~EditorWorkspaceObj()
{
    partition.visit_leaves(partition.get_root_id(), [](EditorWorkspaceNode* node) {
        if (!node->tabControl || !node->window)
            return;

        sEditorWindow[(int)node->window.type()].destroy(node->window);
        sEditorWindow[EDITOR_WINDOW_TAB_CONTROL].destroy(node->tabControl);
    });
}

void EditorWorkspaceObj::set_split_ratio(EditorAreaID areaID, float ratio)
{
    partition.set_split_ratio(areaID, ratio);

    partition.visit_leaves(areaID, [](EditorWorkspaceNode* node) {
        if (!node->tabControl || !node->window)
            return;

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
        if (!node->tabControl || !node->window)
            return;

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
        if (!node->tabControl || !node->window)
            return;

        Rect tabControlRect, windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControl.set_pos(tabControlRect.get_pos());
        node->window.set_pos(windowRect.get_pos());
    });
}

//
// Public API
//

EditorWorkspace EditorWorkspace::create(const EditorWorkspaceInfo& spaceI)
{
    LD_ASSERT(spaceI.uiLayerName && spaceI.uiContextName);

    auto* obj = heap_new<EditorWorkspaceObj>(MEMORY_USAGE_UI, spaceI.rootRect);
    obj->ctx = spaceI.ctx;
    obj->uiContextName = std::string(spaceI.uiContextName);
    obj->uiLayerName = std::string(spaceI.uiLayerName);
    obj->isFloat = spaceI.isFloat;
    obj->isVisible = spaceI.isVisible;
    obj->rootColor = spaceI.rootColor;
    obj->rootRect = spaceI.rootRect;

    EditorWorkspace space = EditorWorkspace(obj);
    space.set_visible(spaceI.isVisible);

    return space;
}

void EditorWorkspace::destroy(EditorWorkspace space)
{
    auto* obj = space.unwrap();

    ui_imgui_cleanup_layer(obj->uiContextName.c_str(), obj->uiLayerName.c_str());

    heap_delete<EditorWorkspaceObj>(obj);
}

bool EditorWorkspace::should_close()
{
    return mObj->shouldClose;
}

void EditorWorkspace::set_visible(bool isVisible)
{
    mObj->isVisible = isVisible;
}

EditorWindow EditorWorkspace::create_window(EditorAreaID areaID, EditorWindowType type)
{
    EditorWorkspaceNode* node = mObj->partition.get_node(areaID);

    if (node->window)
        destroy_window(node->window);

    if (node->tabControl)
        destroy_window(node->tabControl);

    std::string windowName = sEditorWindow[(int)type].defaultName;
    std::string uiWorkspaceName = windowName + std::to_string(node->nodeID);
    std::string tabWindowName(windowName);
    EditorIcon windowIcon = sEditorWindow[(int)type].icon;

    EditorWindowInfo windowI{};
    windowI.ctx = mObj->ctx;
    windowI.type = type;
    windowI.name = windowName.c_str();
    windowI.uiWorkspaceName = uiWorkspaceName.c_str();
    node->window = editor_window_create(windowI);

    windowName += "Tab";
    uiWorkspaceName += "Tab";
    windowI.type = EDITOR_WINDOW_TAB_CONTROL;
    windowI.name = windowName.c_str();
    windowI.uiWorkspaceName = uiWorkspaceName.c_str();
    node->tabControl = editor_window_create(windowI);

    TabControlWindow tabControl = (TabControlWindow)node->tabControl;
    tabControl.set_window_type(type, node->window.get_name().c_str(), windowIcon);

    return node->window;
}

void EditorWorkspace::destroy_window(EditorWindow window)
{
    const std::string& uiContextName = mObj->uiContextName;
    const std::string& uiLayerName = mObj->uiLayerName;
    const std::string& uiWorkspaceName = window.get_ui_workspace_name();
    ui_imgui_cleanup_workspace(uiContextName.c_str(), uiLayerName.c_str(), uiWorkspaceName.c_str());

    editor_window_destroy(window);
}

void EditorWorkspace::pre_update(const EditorUpdateTick& tick)
{
    Vector<EditorAreaID> toClose;
    EditorAreaID rootID = mObj->partition.get_root_id();

    mObj->partition.visit_leaves(rootID, [&](EditorWorkspaceNode* node) {
        if (!node->window || !node->tabControl)
            return;

        if (node->window.should_close())
            toClose.push_back(node->nodeID);
        else
        {
            editor_window_pre_update(node->window.unwrap(), tick);

            TabControlWindow tab = (TabControlWindow)node->tabControl;
            tab.set_window_name(node->window.get_name().c_str());
        }
    });

    if (!toClose.empty())
    {
        LD_ASSERT(toClose.size() == 1 && toClose[0] == rootID);
        mObj->shouldClose = true;
    }
}

void EditorWorkspace::update(const EditorUpdateTick& tick)
{
    LD_PROFILE_SCOPE;

    if (mObj->shouldClose)
        return;

    // each EditorWindow contains a UIWorkspaces, but they all belong to the same UILayer.
    ui_layer_begin(mObj->uiLayerName.c_str());
    ui_layer_set_visible(mObj->isVisible);

    // EditorWorkspace root window detects resizing.
    ui_workspace_begin("WORKSPACE_ROOT", mObj->rootRect);
    ui_push_window("WORKSPACE_ROOT_WINDOW");
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

    if (mObj->control.dragSplitID)
        eui_set_window_cursor(mObj->control.dragSplitAxis == AXIS_X ? CURSOR_TYPE_VRESIZE : CURSOR_TYPE_HRESIZE);
    else if (mObj->control.hoverSplitID)
        eui_set_window_cursor(mObj->control.hoverSplitAxis == AXIS_X ? CURSOR_TYPE_VRESIZE : CURSOR_TYPE_HRESIZE);

    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (EditorWorkspaceObj*)user;
        EditorTheme theme = obj->ctx.get_theme();

        if (obj->rootColor != 0)
        {
            renderer.draw_rect(obj->rootRect, obj->rootColor);
        }

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

    // imgui pass for all EditorWindow in this EditorWorkspace
    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
        if (!node->tabControl || !node->window)
            return;

        Rect tabControlRect;
        Rect windowRect;
        node->get_workspace_rects(tabControlRect, windowRect);
        node->tabControl.set_rect(tabControlRect);
        editor_window_update(node->tabControl.unwrap(), tick);
        node->window.set_rect(windowRect);
        editor_window_update(node->window.unwrap(), tick);

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

Vector<EditorAreaID> EditorWorkspace::post_update()
{
    Vector<EditorAreaID> ids;
    int leafCount = 0;

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [&](EditorWorkspaceNode* node) {
        leafCount++;

        if (!node->tabControl || !node->window)
            return;

        if (node->window.should_close())
        {
            ids.push_back(node->nodeID);

            destroy_window(node->window);
            node->window = {};
            destroy_window(node->tabControl);
            node->tabControl = {};
        }
    });

    if (leafCount == 1 && ids.size() == 1)
        mObj->shouldClose = true;

    return ids;
}

void EditorWorkspace::set_rect(const Rect& rect)
{
    if (mObj->rootRect == rect)
        return; // skip invalidation with epsilon tolerance

    mObj->set_rect(rect);
}

Rect EditorWorkspace::get_rect()
{
    return mObj->rootRect;
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
