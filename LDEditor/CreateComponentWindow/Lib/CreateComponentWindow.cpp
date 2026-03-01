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
    EditorTheme theme;
    CUID parentID = 0;
    int selectedRowIndex = -1;

    CreateComponentWindowObj() = delete;
    CreateComponentWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info) {}

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_CREATE_COMPONENT; }
    virtual void on_imgui(float delta) override;

    void component_rows();
    void component_row(ComponentType type, int rowIndex);
    void on_row_mouse_down(MouseValue mouseVal, const Vec2& mousePos, ComponentType compType);
};

void CreateComponentWindowObj::on_imgui(float delta)
{
    theme = mCtx.get_theme();

    ui_workspace_begin();
    ui_push_window("ROOT");
    ui_top_layout(mCtx.make_editor_window_layout(mRootRect.get_size()));
    ui_window_set_color(theme.get_ui_theme().get_surface_color());
    {
        component_rows();
    }
    ui_pop_window();
    ui_workspace_end();
}

void CreateComponentWindowObj::component_rows()
{
    UITheme uiTheme = theme.get_ui_theme();

    Array<ComponentType, 3> types = {
        COMPONENT_TYPE_AUDIO_SOURCE,
        COMPONENT_TYPE_TRANSFORM_2D,
        COMPONENT_TYPE_SPRITE_2D,
    };

    for (int i = 0; i < (int)types.size(); i++)
    {
        component_row(types[i], i);
    }
}

void CreateComponentWindowObj::component_row(ComponentType type, int rowIndex)
{
    EditorTheme theme = mCtx.get_settings().get_theme();
    UITheme uiTheme = theme.get_ui_theme();
    const float rowHeight = theme.get_text_row_height();
    MouseValue mouseVal;
    Vec2 mousePos;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_padding();
    layoutI.childPadding.left = 10.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);

    ui_push_panel();
    ui_top_layout(layoutI);

    Color panelColor = uiTheme.get_surface_color();
    if (rowIndex == selectedRowIndex)
        panelColor = theme.get_ui_theme().get_selection_color();

    if (rowIndex % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = Color::lift(panelColor, 0.06f);

    ui_panel_color(panelColor);
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(mouseVal, mousePos, type);

    // component icon
    EditorIcon icon = EditorIconAtlas::get_component_icon(type);
    if (icon != EDITOR_ICON_ENUM_LAST)
    {
        const Rect iconRect = EditorIconAtlas::get_icon_rect(icon);
        ui_push_image(mCtx.get_editor_icon_atlas(), rowHeight, rowHeight, 0xFFFFFFFF, &iconRect);
        ui_pop();
    }

    // component name
    ui_push_text(get_component_type_name(type));
    if (ui_top_mouse_down(mouseVal, mousePos))
        on_row_mouse_down(mouseVal, mousePos, type);
    ui_pop();

    ui_pop();
}

void CreateComponentWindowObj::on_row_mouse_down(MouseValue mouseVal, const Vec2& mousePos, ComponentType compType)
{
    if (mouseVal.button() == MOUSE_BUTTON_RIGHT)
    {
        mCtx.action_add_component(parentID, compType);
        mShouldClose = true;
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

void CreateComponentWindow::set_parent_component(CUID parentID)
{
    mObj->parentID = parentID;
}

} // namespace LD
