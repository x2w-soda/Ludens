#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorWidget/EUIRow.h>
#include <LudensEditor/ProjectWindow/ProjectWindow.h>

#define PAD 10.0f

namespace LD {

static bool ui_row_text_button(const char* labelText, const char* buttonText, float rowHeight)
{
    bool btnIsPressed = false;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        ui_push_text(nullptr, labelText);
        ui_pop();

        ui_push_panel(nullptr);
        ui_top_layout(layoutI);
        ui_pop();

        layoutI.sizeX = UISize::fixed(120.0f);
        ui_push_button(nullptr, buttonText);
        ui_top_layout(layoutI);
        if (ui_button_is_pressed())
            btnIsPressed = true;
        ui_pop();
    }
    ui_pop();

    return btnIsPressed;
}

static bool ui_row_text_edit_button(UITextEditStorage* storage, const char* buttonText, float rowHeight)
{
    bool btnIsPressed = false;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = PAD;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(rowHeight);

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        ui_push_text_edit(storage);
        ui_top_layout(layoutI);
        ui_pop();

        layoutI.sizeX = UISize::fixed(100.0f);
        ui_push_button(nullptr, buttonText);
        ui_top_layout(layoutI);
        if (ui_button_is_pressed())
            btnIsPressed = true;
        ui_pop();
    }
    ui_pop();

    return btnIsPressed;
}

struct SelectProjectStorage
{
    UIScrollStorage projectListScroll;
    UITextEditStorage projectSchemaEdit;

    void ui_project_entry(EditorContext ctx, const EditorProjectEntry& entry);
};

void SelectProjectStorage::ui_project_entry(EditorContext ctx, const EditorProjectEntry& entry)
{
    EditorTheme theme = ctx.get_theme();

    ui_push_panel(nullptr);
    ui_top_layout(theme.make_vbox_layout());

    ui_push_text(nullptr, entry.projectName.c_str());
    ui_pop();

    const FS::Path projectDir = entry.schemaPath.parent_path();
    ui_push_text(nullptr, projectDir.string().c_str());
    ui_pop();

    ui_push_button(nullptr, "Load");
    if (ui_button_is_pressed())
    {
        auto* actionE = (EditorActionOpenProjectEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT);
        actionE->projectSchema = entry.schemaPath;
    }
    ui_pop();

    ui_pop();
}

struct CreateProjectStorage
{
    UITextEditStorage projectNameEdit;
    UITextEditStorage projectDirEdit;
    std::string projectNameErr;
    std::string projectDirErr;

    void validate_input();
    inline bool is_valid_input() { return projectNameErr.empty() && projectDirErr.empty(); }
};

void CreateProjectStorage::validate_input()
{
    std::string projectName = projectNameEdit.editor.get_string();
    FS::Path projectDir(projectDirEdit.editor.get_string());
    bool parentDirExists = FS::is_directory(projectDir.parent_path());
    bool projectDirExists = FS::is_directory(projectDir);

    projectNameErr.clear();
    if (projectName.empty())
        projectNameErr = "Empty project name";

    projectDirErr.clear();
    if (!parentDirExists)
        projectDirErr = "Parent directory for project does not exist";
    else if (projectDirExists && !FS::is_empty_directory(projectDir))
        projectDirErr = "Project directory exists and is not empty";
}

struct ProjectWindowObj : EditorWindowObj
{
    CreateProjectStorage createProject;
    SelectProjectStorage selectProject;
    bool isCreatingProject = false;

    ProjectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        createProject.projectDirEdit.set_text(FS::current_path().string());
        createProject.validate_input();
    }

    void update(float delta);
    bool ui_create_project();
    bool ui_select_project();
};

void ProjectWindowObj::update(float delta)
{
    (void)delta;

    begin_update_window();

    UILayoutInfo layoutI = theme.make_vbox_layout_fixed(rootRect.get_size());
    layoutI.childPadding = UIPadding(PAD);
    layoutI.childGap = PAD;
    ui_top_layout(layoutI);

    if (isCreatingProject)
    {
        if (ui_create_project())
            isCreatingProject = false;
    }
    else
    {
        if (ui_select_project())
            isCreatingProject = true;
    }

    end_update_window();
}

bool ProjectWindowObj::ui_create_project()
{
    const float textRowHeight = theme.get_text_row_height();
    UITextStorage* text;
    Color errorColor;
    std::string str;

    theme.get_error_color(errorColor);

    if (ui_row_text_button("Create New Project", "Open Existing", textRowHeight))
        return true;

    ui_push_text(nullptr, "Project Name");
    ui_pop();
    ui_push_text_edit(&createProject.projectNameEdit);
    ui_top_layout_size(UISize::grow(), UISize::fixed(textRowHeight));
    if (ui_text_edit_changed(str) || ui_text_edit_submitted(str))
        createProject.validate_input();
    ui_pop();
    text = ui_push_text(nullptr, createProject.projectNameErr.c_str());
    text->set_fg_color(errorColor);
    ui_pop();

    ui_push_text(nullptr, "Project Directory");
    ui_pop();
    ui_push_text_edit(&createProject.projectDirEdit);
    ui_top_layout_size(UISize::grow(), UISize::fixed(textRowHeight));
    if (ui_text_edit_changed(str) || ui_text_edit_submitted(str))
        createProject.validate_input();
    ui_pop();
    text = ui_push_text(nullptr, createProject.projectDirErr.c_str());
    text->set_fg_color(errorColor);
    ui_pop();

    static UIButtonStorage sBtn[2];
    // sBtn[0].text = "Cancel";
    sBtn[1].text = "Create";
    sBtn[1].isEnabled = createProject.is_valid_input();
    int btnPressed = eui_row_btn_btn(nullptr, sBtn + 1);
    if (btnPressed == 2)
    {
        auto* event = (EditorActionCreateProjectEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT);
        event->projectName = createProject.projectNameEdit.editor.get_string();
        event->projectSchema = FS::Path(createProject.projectDirEdit.editor.get_string()) / FS::Path("project.toml");
    }

    return false;
}

bool ProjectWindowObj::ui_select_project()
{
    const float textRowHeight = theme.get_text_row_height();
    std::string str;

    if (ui_row_text_button("Select Project", "Create New", textRowHeight))
        return true;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.childPadding = UIPadding(theme.get_child_pad_large());

    ui_push_scroll(&selectProject.projectListScroll);
    selectProject.projectListScroll.bgColor = theme.get_ui_theme().get_surface_color();
    ui_top_layout(layoutI);

    for (const EditorProjectEntry& entry : ctx.get_project_entries())
    {
        selectProject.ui_project_entry(ctx, entry);
    }
    ui_pop();

    ui_push_text(nullptr, "Locate Project");
    ui_top_layout_size(UISize::wrap(), UISize::fixed(textRowHeight));
    ui_pop();

    if (ui_row_text_edit_button(&selectProject.projectSchemaEdit, "Add", textRowHeight))
        ;

    return false;
}

//
// Public API
//

EditorWindow ProjectWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<ProjectWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void ProjectWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<ProjectWindowObj*>(window.unwrap());

    heap_delete<ProjectWindowObj>(obj);
}

void ProjectWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<ProjectWindowObj*>(base);

    obj->update(tick.delta);
}

} // namespace LD