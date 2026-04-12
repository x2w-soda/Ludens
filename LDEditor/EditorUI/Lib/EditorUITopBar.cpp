#include <Ludens/DSA/Array.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorUI/EditorUITopBar.h>
#include <LudensEditor/EditorWidget/EUIListMenu.h>
#include <LudensEditor/EditorWidget/EUIPrimitiveEdit.h>
#include <LudensEditor/EditorWidget/EUIText.h>

#include "EditorUIDef.h"

#include <cstddef>
#include <cstdio>

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
    float barHeight;
    Rect barRect;
    EUITextStorage textFile;
    EUITextStorage textEdit;
    EUITextStorage textAbout;
    EUITextStorage textScene;
    EUITextStorage textDocs;

    void invalidate_screen_size(Vec2 screenSize);
    void update();
    void file_menu_window();
    void edit_menu_window();
    void about_menu_window();
};

void EditorTopBarObj::invalidate_screen_size(Vec2 screenSize)
{
    barRect = Rect(0.0f, 0.0f, screenSize.x, barHeight);
}

void EditorTopBarObj::update()
{
    Vec2 mousePos;
    MouseValue mouseVal;
    Rect rect;

    ui_layer_begin(layerName);
    ui_workspace_begin(EDITOR_TOP_BAR_WORKSPACE_NAME, barRect);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fixed(barRect.w);
    layoutI.sizeY = UISize::fixed(barRect.h);

    ui_push_window(EDITOR_TOP_BAR_WINDOW_NAME);
    ui_top_layout(layoutI);

    const float height = ctx.get_theme().get_text_row_height();

    if (eui_text(textFile, "File", height, &rect))
        ui_request_popup_window(EDITOR_TOP_BAR_MENU_FILE_NAME, rect.get_pos_bl());

    if (eui_text(textEdit, "Edit", height, &rect))
        ui_request_popup_window(EDITOR_TOP_BAR_MENU_EDIT_NAME, rect.get_pos_bl());

    if (eui_text(textAbout, "About", height, &rect))
        ui_request_popup_window(EDITOR_TOP_BAR_MENU_ABOUT_NAME, rect.get_pos_bl());

    ui_push_panel(nullptr);
    layoutI.sizeX = UISize::grow();
    ui_top_layout(layoutI);
    ui_pop();

    EditorRequestWorkspaceLayoutEvent* requestE;

    if (eui_text(textScene, "Scene", height, &rect))
    {
        requestE = (EditorRequestWorkspaceLayoutEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_WORKSPACE_LAYOUT);
        requestE->layout = EDITOR_UI_MAIN_LAYOUT_SCENE;
    }

    if (eui_text(textDocs, "Docs", height, &rect))
    {
        requestE = (EditorRequestWorkspaceLayoutEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_WORKSPACE_LAYOUT);
        requestE->layout = EDITOR_UI_MAIN_LAYOUT_DOCS;
    }

    ui_pop_window();

    ui_workspace_end();
    ui_layer_end();

    file_menu_window();
    edit_menu_window();
    about_menu_window();
}

void EditorTopBarObj::file_menu_window()
{
    if (!ui_push_popup_window(EDITOR_TOP_BAR_MENU_FILE_NAME))
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
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE);
        break;
    case FILE_MENU_OPEN_SCENE:
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE);
        break;
    case FILE_MENU_SAVE_SCENE:
    {
        auto* actionE = (EditorActionSaveEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_SAVE);
        actionE->saveSceneSchema = true;
        break;
    }
    case FILE_MENU_NEW_PROJECT:
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT);
        break;
    case FILE_MENU_OPEN_PROJECT:
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT);
        break;
    default:
        break;
    }

    ui_pop_window();
}

void EditorTopBarObj::edit_menu_window()
{
    if (!ui_push_popup_window(EDITOR_TOP_BAR_MENU_EDIT_NAME))
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
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_UNDO);
        break;
    case EDIT_MENU_REDO:
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_REDO);
        break;
    case EDIT_MENU_PROJECT_SETTINGS:
        (void)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS);
        break;
    default:
        break;
    }

    ui_pop_window();
}

void EditorTopBarObj::about_menu_window()
{
    if (!ui_push_popup_window(EDITOR_TOP_BAR_MENU_ABOUT_NAME))
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
    obj->layerName = barI.layerName;
    obj->barHeight = barI.barHeight;
    obj->invalidate_screen_size(barI.screenSize);

    return EditorUITopBar(obj);
}

void EditorUITopBar::destroy(EditorUITopBar topBar)
{
    auto* obj = topBar.unwrap();

    heap_delete<EditorTopBarObj>(obj);
}

void EditorUITopBar::pre_update(const EditorUpdateTick& tick)
{
    mObj->invalidate_screen_size(tick.screenSize);
}

void EditorUITopBar::update(const EditorUpdateTick& tick)
{
    (void)tick;

    mObj->update();
}

} // namespace LD