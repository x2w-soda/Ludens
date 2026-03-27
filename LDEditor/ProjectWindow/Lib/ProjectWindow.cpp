#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/ProjectWindow/ProjectWindow.h>

namespace LD {

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

    ProjectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        createProject.projectDirEdit.set_text(FS::current_path().string());
        createProject.validate_input();
    }

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_PROJECT; }
    virtual void on_imgui(float delta) override;

    void create_project_imgui();
};

void ProjectWindowObj::on_imgui(float delta)
{
    (void)delta;

    EditorTheme theme = mCtx.get_theme();

    ui_workspace_begin();
    ui_push_window(ui_workspace_name());

    UILayoutInfo layoutI = theme.make_vbox_layout_fixed(mRootRect.get_size());
    layoutI.childPadding = UIPadding(10.0f);
    layoutI.childGap = 10.0f;
    ui_top_layout(layoutI);

    create_project_imgui();

    ui_pop_window();
    ui_workspace_end();
}

void ProjectWindowObj::create_project_imgui()
{
    EditorTheme theme = mCtx.get_theme();
    const float textRowHeight = theme.get_text_row_height();
    UITextStorage* text;
    std::string str;

    ui_push_text(nullptr, "Project Name");
    ui_pop();
    ui_push_text_edit(&createProject.projectNameEdit);
    ui_top_layout_size(UISize::grow(), UISize::fixed(textRowHeight));
    if (ui_text_edit_changed(str) || ui_text_edit_submitted(str))
        createProject.validate_input();
    ui_pop();
    text = ui_push_text(nullptr, createProject.projectNameErr.c_str());
    theme.get_error_color(text->fgColor);
    ui_pop();

    ui_push_text(nullptr, "Project Directory");
    ui_pop();
    ui_push_text_edit(&createProject.projectDirEdit);
    ui_top_layout_size(UISize::grow(), UISize::fixed(textRowHeight));
    if (ui_text_edit_changed(str) || ui_text_edit_submitted(str))
        createProject.validate_input();
    ui_pop();
    text = ui_push_text(nullptr, createProject.projectDirErr.c_str());
    theme.get_error_color(text->fgColor);
    ui_pop();

    UIButtonStorage* btn = ui_push_button(nullptr, "Create");
    btn->isEnabled = createProject.is_valid_input();
    if (ui_button_is_pressed())
    {
        auto* event = (EditorActionCreateProjectEvent*)mCtx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT);
        event->projectName = createProject.projectNameEdit.editor.get_string();
        event->projectDir = FS::Path(createProject.projectDirEdit.editor.get_string());
    }
    ui_pop();
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

} // namespace LD