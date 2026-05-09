#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
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
    RImage editorIconAtlas{};
    Section section = SECTION_STARTUP;

    ProjectSettingsWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        ctx = info.ctx;
    }

    void update(float delta);
    void section_names();
    void section_startup();
    void section_rendering();
    void section_screen_layers();

    inline UILayoutInfo make_row_layout()
    {
        UILayoutInfo layoutI{};
        layoutI.childAxis = UI_AXIS_X;
        layoutI.childGap = theme.get_child_gap();
        layoutI.sizeX = UISize::fit();
        layoutI.sizeY = UISize::fit();

        return layoutI;
    }
};

void ProjectSettingsWindowObj::update(float delta)
{
    editorIconAtlas = ctx.get_editor_icon_atlas();
    const UILayoutInfo vboxLayoutI = theme.make_vbox_layout();
    Color bgColor = theme.get_ui_theme().get_field_color();

    begin_update_window();

    ui_top_layout_child_axis(UI_AXIS_X);

    auto* panelS = (UIPanelData*)ui_push_panel(nullptr).get_data();
    panelS->color = bgColor;
    ui_top_layout(vboxLayoutI);
    section_names();
    ui_pop();

    ui_push_panel(nullptr);
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

    end_update_window();
}

void ProjectSettingsWindowObj::section_names()
{
    MouseValue mouseVal;
    Vec2 mousePos;

    ui_push_text(nullptr, "Startup");
    if (ui_top_mouse_down(mouseVal, mousePos) && section != SECTION_STARTUP)
    {
        section = SECTION_STARTUP;
    }
    ui_pop();

    ui_push_text(nullptr, "Rendering");
    if (ui_top_mouse_down(mouseVal, mousePos) && section != SECTION_RENDERING)
    {
        section = SECTION_RENDERING;
    }
    ui_pop();

    ui_push_text(nullptr, "Screen Layers");
    if (ui_top_mouse_down(mouseVal, mousePos) && section != SECTION_SCREEN_LAYERS)
    {
        section = SECTION_SCREEN_LAYERS;
    }
    ui_pop();
}

void ProjectSettingsWindowObj::section_startup()
{
    ProjectStartupSettings startupS = ctx.get_project_settings().startup_settings();
    const float rowHeight = theme.get_text_row_height();
    const float propNameWidth = theme.get_text_label_width();
    const UILayoutInfo layoutI = make_row_layout();
    UITextEditData* edit = nullptr;

    String text;

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        ui_push_text(nullptr, "Window Width");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        edit = (UITextEditData*)ui_push_text_edit(nullptr).get_data();
        edit->set_domain(UI_TEXT_EDIT_DOMAIN_UINT);
        // TODO: if (sectionDirty)
        if (ui_text_edit_submitted(text))
        {
            uint32_t width = (uint32_t)std::stoul(std::string((const char*)text.data(), text.size()));
            if (width > 0)
                startupS.set_window_width(width);
        }
        ui_pop();
    }
    ui_pop();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        ui_push_text(nullptr, "Window Height");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        edit = (UITextEditData*)ui_push_text_edit(nullptr).get_data();
        edit->set_domain(UI_TEXT_EDIT_DOMAIN_UINT);
        // TODO: if (sectionDirty)
        if (ui_text_edit_submitted(text))
        {
            uint32_t height = (uint32_t)std::stoul(std::string((const char*)text.data(), text.size()));
            if (height > 0)
                startupS.set_window_height(height);
        }
        ui_pop();
    }
    ui_pop();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        String name = startupS.get_window_name();

        ui_push_text(nullptr, "Window Name");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        edit = (UITextEditData*)ui_push_text_edit(nullptr).get_data();
        edit->set_domain(UI_TEXT_EDIT_DOMAIN_STRING);
        if (!ui_text_edit_is_editing())
            edit->set_text(name);
        if (ui_text_edit_submitted(name))
            startupS.set_window_name(name);
        ui_pop();
    }
    ui_pop();
}

void ProjectSettingsWindowObj::section_rendering()
{
    ProjectRenderingSettings renderingS = ctx.get_project_settings().rendering_settings();

    ui_push_text(nullptr, "Rendering");
    // TODO: default clear color Vec4, color picker or maybe just text edit for now.
    ui_pop();
}

void ProjectSettingsWindowObj::section_screen_layers()
{
    ProjectScreenLayerSettings screenLayerS = ctx.get_project_settings().screen_layer_settings();
    Vector<ProjectScreenLayer> layers = screenLayerS.get_layers();
    UITextEditData* edit = nullptr;
    SUIDRegistry idReg = ctx.get_suid_registry();

    const UILayoutInfo layoutI = make_row_layout();

    String name;
    bool isPressed = false;
    bool isScreenLayerDirty = false;

    for (const ProjectScreenLayer& layer : layers)
    {
        ui_push_panel(nullptr);
        ui_top_layout(layoutI);

        // screen layer name
        edit = (UITextEditData*)ui_push_text_edit(nullptr).get_data();
        if (!ui_text_edit_is_editing())
            edit->set_text(layer.name);
        if (ui_text_edit_submitted(name))
        {
            screenLayerS.rename_layer(layer.id, name.c_str());
            isScreenLayerDirty = true;
        }
        ui_pop();

        // removal button
        ui_push_button(nullptr, "X");
        if (ui_button_is_pressed() && layers.size() > 1)
        {
            screenLayerS.destroy_layer(idReg, layer.id);
            isScreenLayerDirty = true;
        }
        ui_pop();

        ui_pop();
    }

    ui_push_button(nullptr, "Add");
    if (ui_button_is_pressed())
    {
        name = std::format("layer {}", layers.size() + 1);
        (void)screenLayerS.create_layer(idReg, name.c_str());
        isScreenLayerDirty = true;
    }
    ui_pop();

    if (isScreenLayerDirty)
    {
        auto* notifyE = (EditorNotifyProjectSettingsDirtyEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY);
        notifyE->dirtyScreenLayers = true;
    }
}

//
// Public API
//

EditorWindow ProjectSettingsWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<ProjectSettingsWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void ProjectSettingsWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<ProjectSettingsWindowObj*>(window.unwrap());

    heap_delete<ProjectSettingsWindowObj>(obj);
}

void ProjectSettingsWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<ProjectSettingsWindowObj*>(base);

    obj->update(tick.delta);
}

} // namespace LD