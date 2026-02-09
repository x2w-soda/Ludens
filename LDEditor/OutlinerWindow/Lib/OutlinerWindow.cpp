#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>

#include <iostream>

#include "ComponentMenu.h"

#define OUTLINER_ROW_ODD_COLOR 0x272727FF
#define OUTLINER_ROW_EVEN_COLOR 0x2B2C2FFF
#define OUTLINER_ROW_LEFT_PADDING 10.0f
#define OUTLINER_ROW_LEFT_PADDING_PER_DEPTH 15.0f

namespace LD {

/// @brief Editor outliner window implementation.
struct OutlinerWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_OUTLINER; }
    virtual void on_imgui(float delta) override;

    void component_rows(Scene::Component comp, int& rowIdx, int depth);
    void component_row(int rowIdx, int depth, SUID compSUID);
    void on_row_mouse_down(MouseButton& btn, SUID compSUID);
};

void OutlinerWindowObj::component_rows(Scene::Component comp, int& rowIdx, int depth)
{
    LD_ASSERT(comp);

    component_row(rowIdx++, depth, comp.suid());

    depth++;

    Vector<Scene::Component> children;
    comp.get_children(children);

    for (Scene::Component child : children)
        component_rows(child, rowIdx, depth);

    depth--;
}

void OutlinerWindowObj::component_row(int rowIdx, int depth, SUID compSUID)
{
    EditorTheme theme = ctx.get_settings().get_theme();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_padding();
    layoutI.childPadding.left = OUTLINER_ROW_LEFT_PADDING + depth * OUTLINER_ROW_LEFT_PADDING_PER_DEPTH;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(theme.get_text_row_height());

    Color panelColor = (rowIdx % 2) ? OUTLINER_ROW_ODD_COLOR : OUTLINER_ROW_EVEN_COLOR;
    if (compSUID && compSUID == ctx.get_selected_component())
        panelColor = theme.get_ui_theme().get_selection_color();

    ui_push_panel(&panelColor);
    ui_top_layout(layoutI);

    MouseButton btn;
    if (ui_top_mouse_down(btn))
        on_row_mouse_down(btn, compSUID);

    ui_push_text(compSUID ? ctx.get_component_name(compSUID) : nullptr);
    if (ui_top_mouse_down(btn))
        on_row_mouse_down(btn, compSUID);
    ui_pop();

    Scene::Component comp = ctx.get_component(compSUID);
    if (comp && comp.get_script_asset_id())
    {
        float iconSize = theme.get_text_row_height();
        Rect iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Code);
        ui_push_image(ctx.get_editor_icon_atlas(), iconSize, iconSize, 0xFFFFFFFF, &iconRect);
        ui_pop();
    }

    ui_pop();
}

void OutlinerWindowObj::on_row_mouse_down(MouseButton& btn, SUID compSUID)
{
    if (btn == MOUSE_BUTTON_LEFT)
        ctx.set_selected_component(compSUID);
    else if (btn == MOUSE_BUTTON_RIGHT)
    {
        // TODO: menu for create component, remove component, etc.
        EditorRequestCreateComponentEvent event(compSUID);

        ctx.request_event(&event);
    }
}

void OutlinerWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    root.set_color(root.get_theme().get_surface_color());
    ui_push_window(root);

    Vector<Scene::Component> sceneRoots;
    ctx.get_scene_roots(sceneRoots);

    int rowIdx = 0;
    int depth = 0;

    for (Scene::Component sceneRoot : sceneRoots)
    {
        component_rows(sceneRoot, rowIdx, depth);
    }

    ui_pop_window();
}

//
// Public API
//

EditorWindow OutlinerWindow::create(const EditorWindowInfo& windowI)
{
    EditorContext ctx = windowI.ctx;
    UILayoutInfo layoutI = ctx.make_vbox_layout();
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childPadding.left = 0;
    layoutI.childPadding.right = 0;
    layoutI.childGap = 0;

    OutlinerWindowObj* obj = heap_new<OutlinerWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), layoutI, {}, nullptr);

    /*
    ComponentMenuInfo menuI{};
    menuI.ctx = wm.get_context();
    menuI.theme = obj->ctx.get_theme();
    menuI.onOptionAddScript = windowI.addScriptToComponent;
    menuI.user = windowI.user;
    menuI.layer = wm.get_ground_layer_hash();
    obj->menu.startup(menuI);
    */

    // create one OutlinerRow for each object in scene
    // obj->invalidate();

    return EditorWindow(obj);
}

void OutlinerWindow::destroy(EditorWindow window)
{
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_OUTLINER);

    auto* obj = static_cast<OutlinerWindowObj*>(window.unwrap());

    // TODO: obj->menu.cleanup();

    heap_delete<OutlinerWindowObj>(obj);
}

} // namespace LD