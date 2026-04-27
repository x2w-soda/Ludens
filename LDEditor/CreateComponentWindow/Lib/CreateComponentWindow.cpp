#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/CreateComponentWindow/CreateComponentWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>

namespace LD {

struct CreateComponentWindowObj : EditorWindowObj
{
    SUID parentSUID = 0;
    int selectedRowIndex = -1;

    CreateComponentWindowObj() = delete;
    CreateComponentWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info) {}

    void update(float delta);
    void component_rows();
    void component_row(ComponentType type, int rowIndex);
    void on_row_mouse_down(MouseValue mouseVal, const Vec2& mousePos, ComponentType compType);
};

void CreateComponentWindowObj::update(float delta)
{
    begin_update_window();

    ui_top_layout(theme.make_vbox_layout_fixed(rootRect.get_size()));
    ui_window_set_color(theme.get_ui_theme().get_surface_color());
    {
        component_rows();
    }

    end_update_window();
}

void CreateComponentWindowObj::component_rows()
{
    UITheme uiTheme = theme.get_ui_theme();

    Array<ComponentType, 4> types = {
        COMPONENT_TYPE_AUDIO_SOURCE,
        COMPONENT_TYPE_TRANSFORM_2D,
        COMPONENT_TYPE_CAMERA_2D,
        COMPONENT_TYPE_SPRITE_2D,
    };

    for (int i = 0; i < (int)types.size(); i++)
    {
        component_row(types[i], i);
    }
}

void CreateComponentWindowObj::component_row(ComponentType type, int rowIndex)
{
    UITheme uiTheme = theme.get_ui_theme();
    const float rowHeight = theme.get_text_row_height();
    MouseValue mouseVal;
    Vec2 mousePos;
    UIPanelData* panelData;
    UIImageData* imageData;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_child_gap();
    layoutI.childPadding.left = 10.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);

    panelData = (UIPanelData*)ui_push_panel(nullptr).get_data();
    ui_top_layout(layoutI);

    Color panelColor = uiTheme.get_surface_color();
    if (rowIndex == selectedRowIndex)
        panelColor = theme.get_ui_theme().get_selection_color();

    if (rowIndex % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = Color::lift(panelColor, 0.06f);

    panelData->color = panelColor;
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(mouseVal, mousePos, type);

    // component icon
    EditorIcon icon = EditorIconAtlas::get_component_icon(type);
    if (icon != EDITOR_ICON_ENUM_LAST)
    {
        imageData = (UIImageData*)ui_push_image(nullptr, rowHeight, rowHeight).get_data();
        imageData->image = ctx.get_editor_icon_atlas();
        imageData->rect = EditorIconAtlas::get_icon_rect(icon);
        ui_pop();
    }

    // component name
    ui_push_text(nullptr, get_component_type_name(type));
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(mouseVal, mousePos, type);
    ui_pop();

    ui_pop();
}

void CreateComponentWindowObj::on_row_mouse_down(MouseValue mouseVal, const Vec2& mousePos, ComponentType compType)
{
    if (mouseVal.button() == MOUSE_BUTTON_RIGHT)
    {
        auto* event = (EditorActionAddComponentEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT);
        event->compType = compType;
        event->parentSUID = parentSUID;
        shouldClose = true;
    }
}

//
// Public API
//

EditorWindow CreateComponentWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<CreateComponentWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void CreateComponentWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<CreateComponentWindowObj*>(window.unwrap());

    heap_delete<CreateComponentWindowObj>(obj);
}

void CreateComponentWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<CreateComponentWindowObj*>(base);

    obj->update(tick.delta);
}

void CreateComponentWindow::set_parent_component(SUID parentSUID)
{
    mObj->parentSUID = parentSUID;
}

} // namespace LD
