#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/CreateComponentWindow/CreateComponentWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>

#include <array>

namespace LD {

struct CreateComponentWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    EditorTheme theme;
    CUID parentID = 0;
    int selectedRowIndex = -1;

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_CREATE_COMPONENT; }
    virtual void on_imgui(float delta) override;

    void component_rows();
    void component_row(ComponentType type, int rowIndex);
    void on_row_mouse_down(MouseButton& btn, ComponentType compType);
};

void CreateComponentWindowObj::on_imgui(float delta)
{
    theme = ctx.get_theme();

    ui_push_window(root);
    ui_top_layout_child_axis(UI_AXIS_Y);
    {
        component_rows();
    }
    ui_pop_window();
}

void CreateComponentWindowObj::component_rows()
{
    std::array<ComponentType, 2> types = {
        COMPONENT_TYPE_AUDIO_SOURCE,
        COMPONENT_TYPE_MESH,
    };

    for (int i = 0; i < (int)types.size(); i++)
    {
        component_row(types[i], i);
    }
}

void CreateComponentWindowObj::component_row(ComponentType type, int rowIndex)
{
    EditorTheme theme = ctx.get_settings().get_theme();
    UITheme uiTheme = theme.get_ui_theme();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_padding();
    layoutI.childPadding.left = 10.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(theme.get_text_row_height());

    MouseButton btn;
    Color panelColor = uiTheme.get_surface_color();
    if (rowIndex == selectedRowIndex)
        panelColor = theme.get_ui_theme().get_selection_color();

    ui_push_panel(&panelColor);
    ui_top_layout(layoutI);
    if (ui_top_mouse_down(btn))
        on_row_mouse_down(btn, type);

    // TODO: component icon

    // component name
    ui_push_text(get_component_type_name(type));
    if (ui_top_mouse_down(btn))
        on_row_mouse_down(btn, type);
    ui_pop();

    ui_pop();
}

void CreateComponentWindowObj::on_row_mouse_down(MouseButton& btn, ComponentType compType)
{
    if (btn == MOUSE_BUTTON_RIGHT)
    {
        ctx.action_add_component(parentID, compType);
        mShouldClose = true;
    }
}

//
// Public API
//

EditorWindow CreateComponentWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<CreateComponentWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), obj->ctx.make_vbox_layout(), {}, nullptr);
    obj->root.set_color(obj->ctx.get_theme().get_ui_theme().get_surface_color());
    // obj->editorIconAtlas = obj->ctx.get_editor_icon_atlas();

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
