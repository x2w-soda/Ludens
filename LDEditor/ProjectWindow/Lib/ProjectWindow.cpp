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

struct ProjectWindowObj;

struct ProjectWindowObj : EditorWindowObj
{
    ProjectWindowMode mode = PROJECT_WINDOW_MODE_SELECT_PROJECT;

    struct SelectProjectStorage
    {
        UIScrollData projectListScroll;
        UITextEditData projectSchemaEdit;

        void reset();
        bool update(ProjectWindowObj* obj);
        void ui_project_entry(EditorContext ctx, const EditorProjectEntry& entry);
    } selectProject;

    struct CreateProjectStorage
    {
        EUIButtonRow<2> buttonRow;
        UITextEditData projectNameEdit;
        UITextEditData projectDirEdit;
        std::string projectNameErr;
        std::string projectDirErr;

        bool update(ProjectWindowObj* obj);
        void validate_input();
        inline bool is_valid_input() { return projectNameErr.empty() && projectDirErr.empty(); }
    } createProject;

    struct SaveProjectStorage
    {
        EUIButtonRow<3> buttonRow;
        EditorWindowType nextWindowType = EDITOR_WINDOW_TYPE_ENUM_COUNT;
        EditorWindowMode nextWindowModeHint = -1;

        void update(ProjectWindowObj* obj);
    } saveProject;

    struct CreateSceneStorage
    {
        EUIScenePathEditRow pathEditRow;
        EUIButtonRow<2> buttonRow;
        std::string scenePath;

        void update(ProjectWindowObj* obj);
    } createScene;

    ProjectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        createProject.projectDirEdit.set_text(FS::current_path().string());
        createProject.validate_input();
    }

    void update();
};

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

bool ProjectWindowObj::CreateProjectStorage::update(ProjectWindowObj* obj)
{
    EditorTheme theme = obj->ctx.get_theme();
    const float textRowHeight = theme.get_text_row_height();
    UITextData* text;
    Color errorColor;
    std::string str;

    theme.get_error_color(errorColor);

    if (ui_row_text_button("Create New Project", "Open Existing", textRowHeight))
        return true;

    ui_push_text(nullptr, "Project Name");
    ui_pop();
    ui_push_text_edit(&projectNameEdit);
    ui_top_layout_size(UISize::grow(), UISize::fixed(textRowHeight));
    if (ui_text_edit_changed(str) || ui_text_edit_submitted(str))
        validate_input();
    ui_pop();
    text = (UITextData*)ui_push_text(nullptr, projectNameErr.c_str()).get_data();
    text->set_fg_color(errorColor);
    ui_pop();

    ui_push_text(nullptr, "Project Directory");
    ui_pop();
    ui_push_text_edit(&projectDirEdit);
    ui_top_layout_size(UISize::grow(), UISize::fixed(textRowHeight));
    if (ui_text_edit_changed(str) || ui_text_edit_submitted(str))
        validate_input();
    ui_pop();
    text = (UITextData*)ui_push_text(nullptr, projectDirErr.c_str()).get_data();
    text->set_fg_color(errorColor);
    ui_pop();

    buttonRow.label[0] = "Cancel";
    buttonRow.label[1] = "Create";
    buttonRow.isEnabled[1] = is_valid_input();
    int btnPressed = buttonRow.update();
    if (btnPressed == 1)
        obj->shouldClose = true;
    else if (btnPressed == 2)
    {
        obj->shouldClose = true;

        auto* event = (EditorActionCreateProjectEvent*)obj->ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT);
        event->projectName = projectNameEdit.get_text();
        event->projectSchema = FS::Path(projectDirEdit.get_text()) / FS::Path("project.toml");
    }

    return false;
}

bool ProjectWindowObj::SelectProjectStorage::update(ProjectWindowObj* obj)
{
    EditorTheme theme = obj->ctx.get_theme();
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

    ui_push_scroll(&projectListScroll);
    projectListScroll.bgColor = theme.get_ui_theme().get_surface_color();
    ui_top_layout(layoutI);

    for (const EditorProjectEntry& entry : obj->ctx.get_project_entries())
    {
        ui_project_entry(obj->ctx, entry);
    }
    ui_pop();

    return false;
}

void ProjectWindowObj::SelectProjectStorage::ui_project_entry(EditorContext ctx, const EditorProjectEntry& entry)
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

void ProjectWindowObj::CreateProjectStorage::validate_input()
{
    std::string projectName = projectNameEdit.get_text();
    FS::Path projectDir(projectDirEdit.get_text());
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

void ProjectWindowObj::SaveProjectStorage::update(ProjectWindowObj* obj)
{
    buttonRow.label[0] = "Discard";
    buttonRow.label[1] = "Cancel";
    buttonRow.label[2] = "Save";
    int btnPressed = buttonRow.update();

    if (btnPressed == 1)
    {
        // TODO: clear edit stack?
        auto* requestE = (EditorRequestShowModalEvent*)obj->ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_SHOW_MODAL);
        requestE->windowType = nextWindowType;
        requestE->windowModeHint = nextWindowModeHint;
    }
    else if (btnPressed == 2)
    {
        obj->shouldClose = true;
    }
    else if (btnPressed == 3)
    {
        auto* requestE = (EditorRequestShowModalEvent*)obj->ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_SHOW_MODAL);
        requestE->windowType = nextWindowType;
        requestE->windowModeHint = nextWindowModeHint;
    }
}

void ProjectWindowObj::CreateSceneStorage::update(ProjectWindowObj* obj)
{
    Project project = obj->ctx.get_project();
    std::string str;

    if (pathEditRow.update(project, scenePath))
        ;

    buttonRow.label[0] = "Cancel";
    buttonRow.label[1] = "Create";
    buttonRow.isEnabled[1] = project.is_scene_uri_path_valid(scenePath, str);
    int btnPressed = buttonRow.update();
    if (btnPressed == 1)
        obj->shouldClose = true;
    else if (btnPressed == 2)
    {
        obj->shouldClose = true;

        auto* event = (EditorActionCreateSceneEvent*)obj->ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_CREATE_SCENE);
        event->scenePath = scenePath;
    }
}

void ProjectWindowObj::update()
{
    begin_update_window();

    UILayoutInfo layoutI = theme.make_vbox_layout_fixed(rootRect.get_size());
    layoutI.childPadding = UIPadding(PAD);
    layoutI.childGap = PAD;
    ui_top_layout(layoutI);

    switch (mode)
    {
    case PROJECT_WINDOW_MODE_CREATE_PROJECT:
        if (createProject.update(this))
            mode = PROJECT_WINDOW_MODE_SELECT_PROJECT;
        break;
    case PROJECT_WINDOW_MODE_SELECT_PROJECT:
        if (selectProject.update(this))
            mode = PROJECT_WINDOW_MODE_CREATE_PROJECT;
        break;
    case PROJECT_WINDOW_MODE_SAVE_PROJECT:
        saveProject.update(this);
        break;
    case PROJECT_WINDOW_MODE_CREATE_SCENE:
        createScene.update(this);
        break;
    default:
        break;
    }

    end_update_window();
}

//
// Public API
//

void ProjectWindow::set_mode(ProjectWindowMode mode)
{
    mObj->mode = mode;

    switch (mObj->mode)
    {
    case PROJECT_WINDOW_MODE_CREATE_PROJECT:
        mObj->createProject = {};
        break;
    case PROJECT_WINDOW_MODE_SELECT_PROJECT:
        mObj->selectProject = {};
        break;
    case PROJECT_WINDOW_MODE_SAVE_PROJECT:
        mObj->saveProject = {};
        break;
    case PROJECT_WINDOW_MODE_CREATE_SCENE:
        mObj->createScene = {};
        break;
    default:
        break;
    }
}

void ProjectWindow::set_save_project_continuation(EditorWindowType windowType, EditorWindowMode modeHint)
{
    mObj->saveProject.nextWindowType = windowType;
    mObj->saveProject.nextWindowModeHint = modeHint;
}

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

    (void)tick;

    obj->update();
}

void ProjectWindow::mode_hint(EditorWindowObj* base, EditorWindowMode mode)
{
    auto* obj = static_cast<ProjectWindowObj*>(base);

    ProjectWindow(obj).set_mode((ProjectWindowMode)mode);
}

} // namespace LD
