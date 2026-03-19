#include <Ludens/DSA/Array.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorUI/EditorUITopBar.h>
#include <LudensEditor/EditorWidget/EUIListMenu.h>

#include <cstddef>
#include <cstdio>

#define TOP_BAR_WORKSPACE_NAME "TOP_BAR_WORKSPACE"
#define TOP_BAR_WINDOW_NAME "TOP_BAR_WINDOW"
#define TOP_BAR_MENU_FILE_NAME "TOP_BAR_MENU_FILE"
#define TOP_BAR_MENU_EDIT_NAME "TOP_BAR_MENU_EDIT"
#define TOP_BAR_MENU_ABOUT_NAME "TOP_BAR_MENU_ABOUT"

namespace LD {

enum FileMenuOption
{
    FILE_MENU_NEW_SCENE = 0,
    FILE_MENU_OPEN_SCENE,
    FILE_MENU_SAVE_SCENE,
    FILE_MENU_NEW_PROJECT,
    FILE_MENU_OPEN_PROJECT,
};

enum EditMenuOption
{
    EDIT_MENU_UNDO = 0,
    EDIT_MENU_REDO,
    EDIT_MENU_PROJECT_SETTINGS,
};

enum AboutMenuOption
{
    ABOUT_MENU_VERSION = 0,
};

struct EditorTopBarObj
{
    EditorContext ctx;
    const char* layerName;
    Rect barRect;
    Rect screenRect;

    void on_imgui(float delta);
    void file_menu_window();
    void edit_menu_window();
    void about_menu_window();
};

void EditorTopBarObj::on_imgui(float delta)
{
    Vec2 mousePos;
    MouseValue mouseVal;
    Rect rect;

    ui_layer_begin(layerName);
    ui_workspace_begin(TOP_BAR_WORKSPACE_NAME, barRect);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 5.0f;
    layoutI.sizeX = UISize::fixed(barRect.w);
    layoutI.sizeY = UISize::fixed(barRect.h);

    ui_push_window(TOP_BAR_WINDOW_NAME);
    ui_top_layout(layoutI);

    ui_push_text(nullptr, "File");
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
    {
        ui_top_get_rect(rect);
        ui_request_popup_window(TOP_BAR_MENU_FILE_NAME, rect.get_pos_bl());
    }
    ui_pop();

    ui_push_text(nullptr, "Edit");
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
    {
        ui_top_get_rect(rect);
        ui_request_popup_window(TOP_BAR_MENU_EDIT_NAME, rect.get_pos_bl());
    }
    ui_pop();

    ui_push_text(nullptr, "About");
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
    {
        ui_top_get_rect(rect);
        ui_request_popup_window(TOP_BAR_MENU_ABOUT_NAME, rect.get_pos_bl());
    }
    ui_pop();
    ui_pop_window();

    ui_workspace_end();
    ui_layer_end();

    file_menu_window();
    edit_menu_window();
    about_menu_window();
}

void EditorTopBarObj::file_menu_window()
{
    if (!ui_push_popup_window(TOP_BAR_MENU_FILE_NAME))
        return;

    Array<const char*, 5> options;
    options[FILE_MENU_NEW_SCENE] = "New Scene";
    options[FILE_MENU_OPEN_SCENE] = "Open Scene";
    options[FILE_MENU_SAVE_SCENE] = "Save Scene";
    options[FILE_MENU_NEW_PROJECT] = "New Project";
    options[FILE_MENU_OPEN_PROJECT] = "Open Project";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
        ui_clear_popup_window();

    switch (opt)
    {
    case FILE_MENU_NEW_SCENE:
    {
        EditorRequestNewSceneEvent event{};
        ctx.request_event(&event);
        break;
    }
    case FILE_MENU_OPEN_SCENE:
    {
        EditorRequestOpenSceneEvent event{};
        ctx.request_event(&event);
        break;
    }
    case FILE_MENU_SAVE_SCENE:
        ctx.action_save_scene(); // just save the scene, no dialogs
        break;
    case FILE_MENU_NEW_PROJECT:
    {
        EditorRequestNewProjectEvent event{};
        ctx.request_event(&event);
        break;
    }
    case FILE_MENU_OPEN_PROJECT:
    {
        EditorRequestOpenProjectEvent event{};
        ctx.request_event(&event);
        break;
    }
    default:
        break;
    }

    ui_pop_window();
}

void EditorTopBarObj::edit_menu_window()
{
    if (!ui_push_popup_window(TOP_BAR_MENU_EDIT_NAME))
        return;

    Array<const char*, 3> options;
    options[EDIT_MENU_UNDO] = "Undo";
    options[EDIT_MENU_REDO] = "Redo";
    options[EDIT_MENU_PROJECT_SETTINGS] = "Project Settings";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
        ui_clear_popup_window();

    switch (opt)
    {
    case EDIT_MENU_UNDO:
        ctx.action_undo();
        break;
    case EDIT_MENU_REDO:
        ctx.action_redo();
        break;
    case EDIT_MENU_PROJECT_SETTINGS:
    {
        EditorRequestProjectSettingsEvent event{};
        ctx.request_event(&event);
        break;
    }
    default:
        break;
    }

    ui_pop_window();
}

void EditorTopBarObj::about_menu_window()
{
    if (!ui_push_popup_window(TOP_BAR_MENU_ABOUT_NAME))
        return;

    Array<const char*, 1> options;
    options[ABOUT_MENU_VERSION] = "Version";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
        ui_clear_popup_window();

    switch (opt)
    {
    case ABOUT_MENU_VERSION:
        // TODO: LD_UNREACHABLE;
        break;
    default:
        break;
    }

    ui_pop_window();
}

//
// Public API
//

EditorUITopBar EditorUITopBar::create(const EditorUITopBarInfo& barI)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 5.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(barI.barHeight);

    auto* obj = heap_new<EditorTopBarObj>(MEMORY_USAGE_UI);
    obj->ctx = barI.ctx;
    obj->layerName = barI.groundLayerName;
    obj->barRect = Rect(0.0f, 0.0f, barI.screenSize.x, barI.barHeight);
    obj->screenRect = Rect(0.0f, 0.0f, barI.screenSize.x, barI.screenSize.y);

    return EditorUITopBar(obj);
}

void EditorUITopBar::destroy(EditorUITopBar topBar)
{
    auto* obj = topBar.unwrap();

    heap_delete<EditorTopBarObj>(obj);
}

void EditorUITopBar::on_imgui(float delta)
{
    mObj->on_imgui(delta);
}

} // namespace LD