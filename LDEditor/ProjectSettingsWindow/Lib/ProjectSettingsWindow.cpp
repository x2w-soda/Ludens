#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/ProjectSettingsWindow/ProjectSettingsWindow.h>

#include <format>

namespace LD {

enum Section
{
    SECTION_STARTUP,
    SECTION_RENDERING,
    SECTION_SCREEN_LAYERS,
};

struct ProjectSettingsWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    RImage editorIconAtlas{};
    EditorTheme theme;
    Section section = SECTION_STARTUP;
    bool sectionDirty = false;

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_PROJECT_SETTINGS; }
    virtual void on_imgui(float delta) override;

    void section_names();
    void section_startup();
    void section_rendering();
    void section_screen_layers();

    inline UILayoutInfo make_row_layout()
    {
        UILayoutInfo layoutI{};
        layoutI.childAxis = UI_AXIS_X;
        layoutI.childGap = theme.get_padding();
        layoutI.sizeX = UISize::fit();
        layoutI.sizeY = UISize::fit();

        return layoutI;
    }
};

void ProjectSettingsWindowObj::on_imgui(float delta)
{
    theme = ctx.get_theme();

    ui_push_window(root);
    ui_top_layout_child_axis(UI_AXIS_X);

    const UILayoutInfo vboxLayoutI = ctx.make_vbox_layout();

    Color bgColor = theme.get_ui_theme().get_field_color();
    ui_push_panel(&bgColor);
    ui_top_layout(vboxLayoutI);
    section_names();
    ui_pop();

    ui_push_panel();
    ui_top_layout(vboxLayoutI);

    switch (section)
    {
    case SECTION_STARTUP:
        section_startup();
        break;
    case SECTION_RENDERING:
        section_rendering();
        break;
    case SECTION_SCREEN_LAYERS:
        section_screen_layers();
        break;
    }
    ui_pop();

    ui_pop_window();
}

void ProjectSettingsWindowObj::section_names()
{
    MouseButton btn;

    ui_push_text("Startup");
    if (ui_top_mouse_down(btn) && section != SECTION_STARTUP)
    {
        section = SECTION_STARTUP;
        sectionDirty = true;
    }
    ui_pop();

    ui_push_text("Rendering");
    if (ui_top_mouse_down(btn) && section != SECTION_RENDERING)
    {
        section = SECTION_RENDERING;
        sectionDirty = true;
    }
    ui_pop();

    ui_push_text("Screen Layers");
    if (ui_top_mouse_down(btn) && section != SECTION_SCREEN_LAYERS)
    {
        section = SECTION_SCREEN_LAYERS;
        sectionDirty = true;
    }
    ui_pop();
}

void ProjectSettingsWindowObj::section_startup()
{
    ProjectStartupSettings startupS = ctx.get_project_settings().get_startup_settings();
    const float rowHeight = theme.get_text_row_height();
    const float propNameWidth = theme.get_text_label_width();
    const UILayoutInfo layoutI = make_row_layout();

    std::string text;

    ui_push_panel();
    ui_top_layout(layoutI);
    {
        ui_push_text("Window Width");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        ui_push_text_edit(UI_TEXT_EDIT_DOMAIN_UINT);
        // TODO: if (sectionDirty)
        if (ui_text_edit_submitted(text))
        {
            uint32_t width = (uint32_t)std::stoul(text);
            if (width > 0)
                startupS.set_window_width(width);
        }
        ui_pop();
    }
    ui_pop();

    ui_push_panel();
    ui_top_layout(layoutI);
    {
        ui_push_text("Window Height");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        ui_push_text_edit(UI_TEXT_EDIT_DOMAIN_UINT);
        // TODO: if (sectionDirty)
        if (ui_text_edit_submitted(text))
        {
            uint32_t height = (uint32_t)std::stoul(text);
            if (height > 0)
                startupS.set_window_height(height);
        }
        ui_pop();
    }
    ui_pop();

    ui_push_panel();
    ui_top_layout(layoutI);
    {
        std::string name = startupS.get_window_name();

        ui_push_text("Window Name");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        ui_push_text_edit(UI_TEXT_EDIT_DOMAIN_STRING);
        if (sectionDirty)
            ui_text_edit_set_text(name);
        if (ui_text_edit_submitted(name))
            startupS.set_window_name(name);
        ui_pop();
    }
    ui_pop();

    sectionDirty = false;
}

void ProjectSettingsWindowObj::section_rendering()
{
    ProjectRenderingSettings renderingS = ctx.get_project_settings().get_rendering_settings();

    ui_push_text("Rendering");
    // TODO: default clear color Vec4, color picker or maybe just text edit for now.
    ui_pop();
}

void ProjectSettingsWindowObj::section_screen_layers()
{
    ProjectScreenLayerSettings screenLayerS = ctx.get_project_settings().get_screen_layer_settings();
    Vector<ProjectScreenLayer> layers = screenLayerS.get_layers();

    const UILayoutInfo layoutI = make_row_layout();

    std::string name;
    bool isPressed = false;
    bool isNextFrameDirty = false;

    for (const ProjectScreenLayer& layer : layers)
    {
        ui_push_panel();
        ui_top_layout(layoutI);

        // screen layer name
        ui_push_text_edit();
        if (sectionDirty)
            ui_text_edit_set_text(layer.name);
        if (ui_text_edit_submitted(name))
        {
            screenLayerS.rename_layer(layer.id, name.c_str());
            sectionDirty = isNextFrameDirty = true;
        }
        ui_pop();

        // removal button
        ui_push_button("X", isPressed);
        if (isPressed)
        {
            screenLayerS.destroy_layer(layer.id);
            sectionDirty = isNextFrameDirty = true;
        }
        ui_pop();

        ui_pop();
    }

    isPressed = false;

    ui_push_button("Add", isPressed);
    if (isPressed)
    {
        name = std::format("layer {}", layers.size() + 1);
        screenLayerS.create_layer(name.c_str());
        sectionDirty = isNextFrameDirty = true;
    }
    ui_pop();

    if (!isNextFrameDirty)
        sectionDirty = false;
}

//
// Public API
//

EditorWindow ProjectSettingsWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<ProjectSettingsWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), obj->ctx.make_vbox_layout(), {}, nullptr);
    obj->root.set_color(obj->ctx.get_theme().get_ui_theme().get_surface_color());
    obj->editorIconAtlas = obj->ctx.get_editor_icon_atlas();

    return EditorWindow(obj);
}

void ProjectSettingsWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<ProjectSettingsWindowObj*>(window.unwrap());

    heap_delete<ProjectSettingsWindowObj>(obj);
}

} // namespace LD