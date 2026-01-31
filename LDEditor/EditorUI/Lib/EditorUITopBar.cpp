#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorUI/EditorUITopBar.h>
#include <LudensEditor/EditorWidget/UIListMenuWidget.h>

#include <array>
#include <cstddef>
#include <cstdio>

namespace LD {

enum TopBarMenu
{
    TOP_BAR_MENU_NONE = 0,
    TOP_BAR_MENU_FILE,
    TOP_BAR_MENU_EDIT,
    TOP_BAR_MENU_ABOUT,
};

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
    UILayer floatLayer;
    UILayer groundLayer;
    UIWorkspace rootWS;
    UIWorkspace floatWS;
    UIWindow menuW;
    TopBarMenu menuType = TOP_BAR_MENU_NONE;
    float barHeight = 0.0f;

    void on_imgui(float delta);
    void file_menu_window();
    void edit_menu_window();
    void about_menu_window();
};

void EditorTopBarObj::on_imgui(float delta)
{
    MouseButton btn;
    UIWindow rootW = rootWS.get_area_window(rootWS.get_root_id());
    Rect rect;

    ui_push_window(rootW);
    ui_push_text("File");
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        ui_top_rect(rect);
        menuType = TOP_BAR_MENU_FILE;
        menuW.set_pos(rect.get_pos_bl());
        menuW.show();
    }
    ui_pop();

    ui_push_text("Edit");
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        ui_top_rect(rect);
        menuType = TOP_BAR_MENU_EDIT;
        menuW.set_pos(rect.get_pos_bl());
        menuW.show();
    }
    ui_pop();

    ui_push_text("About");
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        ui_top_rect(rect);
        menuType = TOP_BAR_MENU_ABOUT;
        menuW.set_pos(rect.get_pos_bl());
        menuW.show();
    }
    ui_pop();
    ui_pop_window();

    file_menu_window();
    edit_menu_window();
    about_menu_window();
}

void EditorTopBarObj::file_menu_window()
{
    if (menuType != TOP_BAR_MENU_FILE)
        return;

    menuW.set_color(0xFF);
    menuW.set_pos(Vec2(0.0f, barHeight));
    ui_push_window(menuW);

    std::array<const char*, 5> options;
    options[FILE_MENU_NEW_SCENE] = "New Scene";
    options[FILE_MENU_OPEN_SCENE] = "Open Scene";
    options[FILE_MENU_SAVE_SCENE] = "Save Scene";
    options[FILE_MENU_NEW_PROJECT] = "New Project";
    options[FILE_MENU_OPEN_PROJECT] = "Open Project";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
        menuW.hide();

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
    if (menuType != TOP_BAR_MENU_EDIT)
        return;

    menuW.set_color(0xFF);
    ui_push_window(menuW);

    std::array<const char*, 3> options;
    options[EDIT_MENU_UNDO] = "Undo";
    options[EDIT_MENU_REDO] = "Redo";
    options[EDIT_MENU_PROJECT_SETTINGS] = "Project Settings";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
        menuW.hide();

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
    if (menuType != TOP_BAR_MENU_ABOUT)
        return;

    menuW.set_color(0xFF);
    ui_push_window(menuW);

    std::array<const char*, 1> options;
    options[ABOUT_MENU_VERSION] = "Version";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
        menuW.hide();

    switch (opt)
    {
    case ABOUT_MENU_VERSION:
        LD_UNREACHABLE; // TODO:
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

    Rect barRect(0.0f, 0.0f, barI.screenSize.x, barI.barHeight);
    Rect screenRect(0.0f, 0.0f, barI.screenSize.x, barI.screenSize.y);

    auto* obj = heap_new<EditorTopBarObj>(MEMORY_USAGE_UI);
    obj->ctx = barI.ctx;
    obj->barHeight = barI.barHeight;
    obj->floatLayer = barI.floatLayer;
    obj->groundLayer = barI.groundLayer;
    obj->rootWS = obj->groundLayer.create_workspace(barRect);
    obj->rootWS.create_window(obj->rootWS.get_root_id(), layoutI, {}, nullptr);

    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childAxis = UI_AXIS_Y;
    obj->floatWS = obj->floatLayer.create_workspace(screenRect);
    obj->menuW = obj->floatWS.create_window(layoutI, {}, nullptr);
    obj->menuW.hide();

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