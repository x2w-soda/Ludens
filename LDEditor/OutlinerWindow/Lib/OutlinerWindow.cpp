#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorWidget/UIListMenuWidget.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>

#define OUTLINER_ROW_LEFT_PADDING 10.0f
#define OUTLINER_ROW_LEFT_PADDING_PER_DEPTH 10.0f
#define OUTLINER_COMPONENT_MENU_POPUP "OUTLINER_COMPONENT_MENU"

#define COMPONENT_MENU_OPTION_ADD_CHILD 0

namespace LD {

/// @brief Outliner frame state
struct OutlinerState
{
    SUID compSUID;
};

/// @brief Editor outliner window implementation.
struct OutlinerWindowObj : EditorWindowObj
{
    RImage editorIconAtlas;
    OutlinerState state{};

    OutlinerWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        editorIconAtlas = mCtx.get_editor_icon_atlas();
    }

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_OUTLINER; }
    virtual void on_imgui(float delta) override;

    void component_rows(ComponentView comp, int& rowIdx, int depth);
    void component_row(int rowIdx, int depth, SUID compSUID);
    void on_row_mouse_down(MouseValue mouseVal, const Vec2& mousePos, SUID compSUID);
};

void OutlinerWindowObj::component_rows(ComponentView comp, int& rowIdx, int depth)
{
    LD_ASSERT(comp);

    component_row(rowIdx++, depth, comp.suid());

    depth++;

    Vector<ComponentView> children;
    comp.get_children(children);

    for (ComponentView child : children)
        component_rows(child, rowIdx, depth);

    depth--;
}

void OutlinerWindowObj::component_row(int rowIdx, int depth, SUID compSUID)
{
    EditorTheme theme = mCtx.get_settings().get_theme();
    UITheme uiTheme = theme.get_ui_theme();
    const float rowHeight = theme.get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_padding();
    layoutI.childPadding.left = OUTLINER_ROW_LEFT_PADDING + depth * OUTLINER_ROW_LEFT_PADDING_PER_DEPTH;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);
    ui_push_panel();
    ui_top_layout(layoutI);

    Color panelColor = uiTheme.get_surface_color();
    if (rowIdx % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = Color::lift(panelColor, 0.06f);

    if (compSUID && compSUID == mCtx.get_selected_component())
        panelColor = theme.get_ui_theme().get_selection_color();

    ui_panel_color(panelColor);

    Vec2 mousePos;
    MouseValue mouseVal;
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(mouseVal, mousePos, compSUID);

    ComponentView comp = mCtx.get_component(compSUID);

    // component type icon
    if (comp)
    {
        EditorIcon icon = EditorIconAtlas::get_component_icon(comp.type());
        if (icon != EDITOR_ICON_ENUM_LAST)
        {
            const Rect iconRect = EditorIconAtlas::get_icon_rect(icon);
            ui_push_image(editorIconAtlas, rowHeight, rowHeight, 0xFFFFFFFF, &iconRect);
            ui_pop();
        }
    }

    // component name label
    ui_push_text(compSUID ? mCtx.get_component_name(compSUID) : nullptr);
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(mouseVal, mousePos, compSUID);
    ui_pop();

    // component script icon
    if (comp && comp.get_script_asset_id())
    {
        float iconSize = theme.get_text_row_height();
        const Rect iconRect = EditorIconAtlas::get_icon_rect(EDITOR_ICON_SCRIPT);
        ui_push_image(mCtx.get_editor_icon_atlas(), iconSize, iconSize, 0xFFFFFFFF, &iconRect);
        ui_pop();
    }

    ui_pop();
}

void OutlinerWindowObj::on_row_mouse_down(MouseValue mouseVal, const Vec2& mousePos, SUID compSUID)
{
    if (mouseVal.button() == MOUSE_BUTTON_LEFT)
        mCtx.set_selected_component(compSUID);
    else if (mouseVal.button() == MOUSE_BUTTON_RIGHT)
    {
        ui_request_popup_window(OUTLINER_COMPONENT_MENU_POPUP, mousePos);
        state.compSUID = compSUID;
    }
}

void OutlinerWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    EditorTheme theme = mCtx.get_theme();
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(mRootRect.w);
    layoutI.sizeY = UISize::fixed(mRootRect.h);
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childPadding.left = 0;
    layoutI.childPadding.right = 0;
    layoutI.childGap = 0;

    ui_workspace_begin();
    ui_push_window("ROOT");
    ui_top_layout(layoutI);
    ui_window_set_color(theme.get_ui_theme().get_surface_color());

    Vector<ComponentView> sceneRoots;
    mCtx.get_scene_roots(sceneRoots);

    int rowIdx = 0;
    int depth = 0;

    for (ComponentView sceneRoot : sceneRoots)
    {
        component_rows(sceneRoot, rowIdx, depth);
    }

    ui_pop_window();

    if (ui_push_popup_window(OUTLINER_COMPONENT_MENU_POPUP))
    {
        Array<const char*, 1> options;
        options[COMPONENT_MENU_OPTION_ADD_CHILD] = "Add Child";

        int opt = eui_list_menu(theme, options.size(), options.data());
        if (opt >= 0)
            ui_clear_popup_window();

        switch (opt)
        {
        case COMPONENT_MENU_OPTION_ADD_CHILD:
        {
            EditorRequestCreateComponentEvent event(state.compSUID);
            mCtx.request_event(&event);
            break;
        }
        default:
            break;
        }

        ui_pop_window();
    }

    ui_workspace_end();
}

//
// Public API
//

EditorWindow OutlinerWindow::create(const EditorWindowInfo& windowI)
{
    OutlinerWindowObj* obj = heap_new<OutlinerWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void OutlinerWindow::destroy(EditorWindow window)
{
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_OUTLINER);

    auto* obj = static_cast<OutlinerWindowObj*>(window.unwrap());

    heap_delete<OutlinerWindowObj>(obj);
}

} // namespace LD