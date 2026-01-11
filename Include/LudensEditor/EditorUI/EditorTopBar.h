#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>

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
};

enum AboutMenuOption
{
    ABOUT_MENU_VERSION = 0,
};

struct EditorTopBarInfo
{
    EditorContext ctx;
    UILayer groundLayer; // top bar live in the ground layer
    UILayer floatLayer;  // list menus live in the float layer
    float barHeight;     // top bar height
    Vec2 screenSize;
};

/// @brief Editor top bar menu UI.
struct EditorTopBar : Handle<struct EditorTopBarObj>
{
    static EditorTopBar create(const EditorTopBarInfo& topBarI);
    static void destroy(EditorTopBar topBar);

    void on_imgui(float delta);
    bool file_menu_option(FileMenuOption& opt);
    bool edit_menu_option(EditMenuOption& opt);
    bool about_menu_option(AboutMenuOption& opt);
};

} // namespace LD