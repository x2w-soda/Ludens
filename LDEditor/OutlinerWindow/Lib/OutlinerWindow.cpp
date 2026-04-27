#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/IndexTable.h>
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

struct OutlinerWindowObj;

/// @brief Outliner frame state
struct OutlinerFrameState
{
    ComponentView selectedComp;
    bool requestCompRename;
};

class OutlinerRow
{
public:
    void update(OutlinerWindowObj* obj, ComponentView comp, int rowIdx, int depth);

private:
    UIPanelData mPanel;
    UIImageData mTypeIcon;
    UIImageData mScriptIcon;
    UITextEditData mLabel;
};

/// @brief Editor outliner window implementation.
struct OutlinerWindowObj : EditorWindowObj
{
    RImage editorIconAtlas;
    OutlinerFrameState state = {};
    SUID parentSUID = {};
    IndexTable<OutlinerRow, MEMORY_USAGE_UI> rows;

    OutlinerWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        editorIconAtlas = ctx.get_editor_icon_atlas();
    }

    bool input_key(KeyValue keyVal);
    void update();
    void component_rows(ComponentView comp, int& rowIdx, int depth);
    void on_row_mouse_down(ComponentView comp, MouseValue mouseVal, const Vec2& mousePos);
};

void OutlinerWindowObj::component_rows(ComponentView comp, int& rowIdx, int depth)
{
    LD_ASSERT(comp);

    rows[rowIdx++]->update(this, comp, rowIdx, depth);

    depth++;

    Vector<ComponentView> children;
    comp.get_children(children);

    for (ComponentView child : children)
        component_rows(child, rowIdx, depth);

    depth--;
}

void OutlinerWindowObj::on_row_mouse_down(ComponentView comp, MouseValue mouseVal, const Vec2& mousePos)
{
    if (mouseVal.button() == MOUSE_BUTTON_LEFT)
        ctx.set_selected_component(comp.cuid());
    else if (mouseVal.button() == MOUSE_BUTTON_RIGHT && comp.suid())
    {
        ui_request_overlay_window(OUTLINER_COMPONENT_MENU_POPUP, 0, mousePos);
        parentSUID = comp.suid();
    }
}

bool OutlinerWindowObj::input_key(KeyValue keyVal)
{
    if (keyVal != KeyValue(KEY_CODE_F2))
        return false;

    state.requestCompRename = true;
    return true;
}

void OutlinerWindowObj::update()
{
    LD_PROFILE_SCOPE;

    UILayoutInfo layoutI(UISize::fixed(rootRect.w), UISize::fixed(rootRect.h), UI_AXIS_Y);
    layoutI.childPadding.left = 0;
    layoutI.childPadding.right = 0;
    layoutI.childGap = 0;

    state = {};
    state.selectedComp = ctx.get_selected_component_view();

    begin_update_window();

    ui_top_layout(layoutI);
    ui_window_set_color(theme.get_ui_theme().get_surface_color());

    Vec2 mousePos;
    MouseValue mouseVal;
    KeyValue keyVal;
    if (ui_top_key_down(keyVal) && !input_key(keyVal))
    {
        ctx.input_key_value(keyVal);
    }
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_RIGHT)
    {
        ui_request_overlay_window(OUTLINER_COMPONENT_MENU_POPUP, 0, mousePos);
        parentSUID = SUID(0);
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

    if (ui_push_overlay_window(OUTLINER_COMPONENT_MENU_POPUP))
    {
        Array<const char*, 1> options;
        options[COMPONENT_MENU_OPTION_ADD_CHILD] = "Add Child";
        if (!parentSUID)
            options[COMPONENT_MENU_OPTION_ADD_CHILD] = "Add Component";

        int opt = eui_list_menu(options.size(), options.data());
        if (opt >= 0)
            ui_clear_overlay_windows();

        switch (opt)
        {
        case COMPONENT_MENU_OPTION_ADD_CHILD:
        {
            auto* event = (EditorRequestCreateComponentEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT);
            event->parent = parentSUID;
            parentSUID = SUID(0);
            break;
        }
        default:
            break;
        }

        ui_pop_window();
    }
}

void OutlinerRow::update(OutlinerWindowObj* obj, ComponentView comp, int rowIdx, int depth)
{
    EditorTheme theme = obj->theme;
    UITheme uiTheme = theme.get_ui_theme();
    const float rowHeight = theme.get_text_row_height();
    CUID compCUID = comp.cuid();
    SUID compSUID = comp.suid();
    RImage iconAtlas = obj->ctx.get_editor_icon_atlas();
    bool isSelectedCompRow = compCUID && obj->state.selectedComp && obj->state.selectedComp.cuid() == compCUID;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_child_pad();
    layoutI.childPadding.left = OUTLINER_ROW_LEFT_PADDING + depth * OUTLINER_ROW_LEFT_PADDING_PER_DEPTH;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);
    ui_push_panel(&mPanel);
    ui_top_layout(layoutI);

    Color panelColor = uiTheme.get_surface_color();
    if (rowIdx % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = uiTheme.get_surface_color_lifted();

    if (isSelectedCompRow)
        panelColor = uiTheme.get_selection_color();

    mPanel.color = panelColor;

    Vec2 mousePos;
    MouseValue mouseVal;
    if (ui_top_mouse_down(mouseVal, mousePos))
        obj->on_row_mouse_down(comp, mouseVal, mousePos);

    // component type icon
    if (comp)
    {
        EditorIcon icon = EditorIconAtlas::get_component_icon(comp.type());
        if (icon != EDITOR_ICON_ENUM_LAST)
        {
            ui_push_image(&mTypeIcon, rowHeight, rowHeight);
            mTypeIcon.image = iconAtlas;
            mTypeIcon.rect = EditorIconAtlas::get_icon_rect(icon);
            ui_pop();
        }
    }

    // component name label
    const char* compName = compCUID ? obj->ctx.get_component_name(compCUID) : nullptr;
    mLabel.bgColor = 0;
    mLabel.beginEditOnFocus = false;
    UITextEditWidget editW = ui_push_text_edit(&mLabel);
    ui_top_layout_size_x(UISize::grow());
    if (isSelectedCompRow && obj->state.requestCompRename)
    {
        obj->state.requestCompRename = false;
        (void)editW.try_begin_edit();
    }
    if (!editW.is_editing() && compName)
        mLabel.set_text(compName);
    std::string name;
    if (ui_text_edit_submitted(name) && !name.empty()) // TODO: name validity check
    {
        auto* actionE = (EditorActionRenameComponentEvent*)obj->ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_RENAME_COMPONENT);
        actionE->compSUID = compSUID;
        actionE->newName = name;
    }
    if (ui_top_mouse_down(mouseVal, mousePos))
        obj->on_row_mouse_down(comp, mouseVal, mousePos);
    ui_pop();

    // component script icon
    if (comp && comp.get_script_asset_id())
    {
        float iconSize = theme.get_text_row_height();
        ui_push_image(&mScriptIcon, iconSize, iconSize);
        mScriptIcon.rect = EditorIconAtlas::get_icon_rect(EDITOR_ICON_SCRIPT);
        mScriptIcon.image = iconAtlas;
        ui_pop();
    }

    ui_pop();
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

    (void)tick;

    obj->update();
}

} // namespace LD