#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorUI/EditorTopBar.h>
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
    int fileMenuOpt = -1;
    int editMenuOpt = -1;
    int aboutMenuOpt = -1;

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
    {
        fileMenuOpt = opt;
        menuW.hide();
    }

    ui_pop_window();
}

void EditorTopBarObj::edit_menu_window()
{
    if (menuType != TOP_BAR_MENU_EDIT)
        return;

    menuW.set_color(0xFF);
    ui_push_window(menuW);

    std::array<const char*, 2> options;
    options[EDIT_MENU_UNDO] = "Undo";
    options[EDIT_MENU_REDO] = "Redo";

    int opt = eui_list_menu(ctx.get_theme(), (int)options.size(), options.data());
    if (opt >= 0)
    {
        editMenuOpt = opt;
        menuW.hide();
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
    {
        aboutMenuOpt = opt;
        menuW.hide();
    }

    ui_pop_window();
}

//
// Public API
//

EditorTopBar EditorTopBar::create(const EditorTopBarInfo& barI)
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

    return EditorTopBar(obj);
}

void EditorTopBar::destroy(EditorTopBar topBar)
{
    auto* obj = topBar.unwrap();

    heap_delete<EditorTopBarObj>(obj);
}

void EditorTopBar::on_imgui(float delta)
{
    mObj->on_imgui(delta);
}

bool EditorTopBar::file_menu_option(FileMenuOption& opt)
{
    if (mObj->fileMenuOpt < 0)
        return false;

    opt = (FileMenuOption)mObj->fileMenuOpt;
    mObj->fileMenuOpt = -1;
    return true;
}

bool EditorTopBar::edit_menu_option(EditMenuOption& opt)
{
    if (mObj->editMenuOpt < 0)
        return false;

    opt = (EditMenuOption)mObj->editMenuOpt;
    mObj->editMenuOpt = -1;
    return true;
}

bool EditorTopBar::about_menu_option(AboutMenuOption& opt)
{
    if (mObj->aboutMenuOpt < 0)
        return false;

    opt = (AboutMenuOption)mObj->aboutMenuOpt;
    mObj->aboutMenuOpt = -1;
    return true;
}

} // namespace LD