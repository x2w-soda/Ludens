#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorWidget/EUIListMenu.h>
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
        editorIconAtlas = ctx.get_editor_icon_atlas();
    }

    void update(float delta);
    void component_rows(ComponentView comp, int& rowIdx, int depth);
    void component_row(ComponentView comp, int rowIdx, int depth);
    void on_row_mouse_down(ComponentView comp, MouseValue mouseVal, const Vec2& mousePos);
};

void OutlinerWindowObj::component_rows(ComponentView comp, int& rowIdx, int depth)
{
    LD_ASSERT(comp);

    component_row(comp, rowIdx++, depth);

    depth++;

    Vector<ComponentView> children;
    comp.get_children(children);

    for (ComponentView child : children)
        component_rows(child, rowIdx, depth);

    depth--;
}

void OutlinerWindowObj::component_row(ComponentView comp, int rowIdx, int depth)
{
    LD_ASSERT(comp);

    EditorTheme theme = ctx.settings().get_theme();
    UITheme uiTheme = theme.get_ui_theme();
    const float rowHeight = theme.get_text_row_height();
    CUID compCUID = comp.cuid();
    SUID compSUID = comp.suid();
    UIImageStorage* imageS;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_child_pad();
    layoutI.childPadding.left = OUTLINER_ROW_LEFT_PADDING + depth * OUTLINER_ROW_LEFT_PADDING_PER_DEPTH;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);
    UIPanelStorage* panelS = ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    Color panelColor = uiTheme.get_surface_color();
    if (rowIdx % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = uiTheme.get_surface_color_lifted();

    if (compCUID && compCUID == ctx.get_selected_component())
        panelColor = theme.get_ui_theme().get_selection_color();

    panelS->color = panelColor;

    Vec2 mousePos;
    MouseValue mouseVal;
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(comp, mouseVal, mousePos);

    // component type icon
    if (comp)
    {
        EditorIcon icon = EditorIconAtlas::get_component_icon(comp.type());
        if (icon != EDITOR_ICON_ENUM_LAST)
        {
            imageS = ui_push_image(nullptr, rowHeight, rowHeight);
            imageS->image = editorIconAtlas;
            imageS->rect = EditorIconAtlas::get_icon_rect(icon);
            ui_pop();
        }
    }

    // component name label
    ui_push_text(nullptr, compCUID ? ctx.get_component_name(compCUID) : nullptr);
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(comp, mouseVal, mousePos);
    ui_pop();

    // component script icon
    if (comp && comp.get_script_asset_id())
    {
        float iconSize = theme.get_text_row_height();
        imageS = ui_push_image(nullptr, iconSize, iconSize);
        imageS->rect = EditorIconAtlas::get_icon_rect(EDITOR_ICON_SCRIPT);
        imageS->image = ctx.get_editor_icon_atlas();
        ui_pop();
    }

    ui_pop();
}

void OutlinerWindowObj::on_row_mouse_down(ComponentView comp, MouseValue mouseVal, const Vec2& mousePos)
{
    if (mouseVal.button() == MOUSE_BUTTON_LEFT)
        ctx.set_selected_component(comp.cuid());
    else if (mouseVal.button() == MOUSE_BUTTON_RIGHT && comp.suid())
    {
        ui_request_popup_window(OUTLINER_COMPONENT_MENU_POPUP, mousePos);
        state.compSUID = comp.suid();
    }
}

void OutlinerWindowObj::update(float delta)
{
    LD_PROFILE_SCOPE;

    EditorTheme theme = ctx.get_theme();
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(rootRect.w);
    layoutI.sizeY = UISize::fixed(rootRect.h);
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childPadding.left = 0;
    layoutI.childPadding.right = 0;
    layoutI.childGap = 0;

    begin_update_window();

    ui_top_layout(layoutI);
    ui_window_set_color(theme.get_ui_theme().get_surface_color());

    KeyValue keyVal;
    if (ui_top_key_down(keyVal))
    {
        ctx.input_key_value(keyVal);
    }

    Vector<ComponentView> sceneRoots;
    ctx.get_scene_roots(sceneRoots);

    int rowIdx = 0;
    int depth = 0;

    for (ComponentView sceneRoot : sceneRoots)
    {
        component_rows(sceneRoot, rowIdx, depth);
    }

    end_update_window();

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
            auto* event = (EditorRequestCreateComponentEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT);
            event->parent = state.compSUID;
            break;
        }
        default:
            break;
        }

        ui_pop_window();
    }
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
    auto* obj = static_cast<OutlinerWindowObj*>(window.unwrap());

    heap_delete<OutlinerWindowObj>(obj);
}

void OutlinerWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<OutlinerWindowObj*>(base);

    obj->update(tick.delta);
}

} // namespace LD