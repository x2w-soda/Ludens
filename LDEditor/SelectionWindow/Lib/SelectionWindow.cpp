#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>

namespace LD {

static bool get_directory_contents_with_filter(const FS::Path& directory, Vector<FS::Path>& contents, const char* extFilter)
{
    std::string err;

    if (!FS::get_directory_content(directory, contents, false, err))
        return false;

    FS::filter_files_by_extension(contents, extFilter);
    return true;
}

struct SelectionWindowObj : EditorWindowObj
{
    RImage editorIconAtlas{};
    EditorTheme theme;
    std::string extensionFilter;
    Vector<FS::Path> directoryContents;
    FS::Path directoryPath;
    FS::Path selectedPath;
    int selectedRowIndex = -1;

    SelectionWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        editorIconAtlas = mCtx.get_editor_icon_atlas();
    }

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_SELECTION; }
    virtual void on_imgui(float delta) override;
    void top_bar();
    void bottom_bar();
    bool row(int idx);
};

void SelectionWindowObj::on_imgui(float delta)
{
    theme = mCtx.get_theme();
    selectedPath.clear();

    ui_workspace_begin();
    ui_push_window(ui_workspace_name());

    ui_top_layout(theme.make_vbox_layout_fixed(mRootRect.get_size()));
    ui_window_set_color(theme.get_ui_theme().get_surface_color());

    top_bar();

    // TODO: is this too frequent for calling Filesystem APIs?
    get_directory_contents_with_filter(directoryPath, directoryContents, extensionFilter.c_str());

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childGap = 2.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    UIScrollStorage* scrollS = ui_push_scroll(nullptr);
    scrollS->bgColor = theme.get_ui_theme().get_surface_color();
    ui_top_layout(layoutI);

    int contentCount = (int)directoryContents.size();

    for (int idx = 0; idx < contentCount; idx++)
    {
        if (row(idx))
            selectedRowIndex = idx;
    }
    ui_pop();

    bottom_bar();
    ui_pop_window();
    ui_workspace_end();
}

void SelectionWindowObj::top_bar()
{
    float fontSize = theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    Vec2 mousePos;
    MouseValue mouseVal;
    UIImageStorage* imageS = ui_push_image(nullptr, fontSize * 1.2f, fontSize * 1.2f);
    imageS->image = editorIconAtlas;
    imageS->rect = EditorIconAtlas::get_icon_rect(EDITOR_ICON_ARROW_UP);
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
    {
        directoryPath = directoryPath.parent_path();
        selectedRowIndex = -1;
    }
    ui_pop();

    std::string text = "Path: ";
    text += directoryPath.string();
    ui_push_text(nullptr, text.c_str());
    ui_pop();

    ui_pop();
}

void SelectionWindowObj::bottom_bar()
{
    float pad = theme.get_child_pad();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childAlignX = UI_ALIGN_END;
    layoutI.childPadding = UIPadding(pad);
    layoutI.childGap = pad;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    ui_push_button(nullptr, "select");
    bool isSelected = ui_button_is_pressed();
    if (isSelected)
    {
        LD_ASSERT(0 <= selectedRowIndex && selectedRowIndex <= (int)directoryContents.size());
        selectedPath = directoryContents[selectedRowIndex];
    }
    ui_pop();

    ui_push_button(nullptr, "cancel");
    bool isCancelled = ui_button_is_pressed();
    ui_pop();

    ui_pop();

    if (isSelected || isCancelled)
        mShouldClose = true;
}

bool SelectionWindowObj::row(int idx)
{
    bool isSelected = false;

    LD_ASSERT(0 <= idx && idx < directoryContents.size());

    Vec2 mousePos;
    MouseValue mouseVal;
    EditorTheme edTheme = mCtx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    float fontSize = edTheme.get_font_size();
    const FS::Path& itemPath = directoryContents[idx];
    float rowHeight = fontSize * 1.2f;
    bool isDirectory = FS::is_directory(itemPath);
    UIImageStorage* imageS;
    UIPanelStorage* panelS;

    Color panelColor = idx == selectedRowIndex ? uiTheme.get_selection_color() : Color(0); // TODO:
    UILayoutInfo layoutI{};
    layoutI.sizeY = UISize::fixed(rowHeight);
    layoutI.sizeX = UISize::grow();
    layoutI.childAxis = UI_AXIS_X;
    panelS = ui_push_panel(nullptr);
    panelS->color = panelColor;
    ui_top_layout(layoutI);

    imageS = ui_push_image(nullptr, rowHeight, rowHeight);
    imageS->image = editorIconAtlas;
    imageS->rect = EditorIconAtlas::get_icon_rect(isDirectory ? EDITOR_ICON_FOLDER : EDITOR_ICON_FILE);
    ui_pop();

    std::string fileString = itemPath.filename().string();
    ui_push_text(nullptr, fileString.c_str());
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
    {
        if (isDirectory)
        {
            directoryPath = itemPath;
            selectedRowIndex = -1;
        }
        else
            isSelected = true;
    }
    ui_pop();

    ui_pop();

    return isSelected;
}

//
// Public API
//

EditorWindow SelectionWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<SelectionWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void SelectionWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<SelectionWindowObj*>(window.unwrap());

    heap_delete<SelectionWindowObj>(obj);
}

void SelectionWindow::show(const FS::Path& directoryPath, const char* extensionFilter)
{
    mObj->directoryPath = directoryPath;
    mObj->directoryContents.clear();
    mObj->extensionFilter = std::string(extensionFilter);
    mObj->selectedPath.clear();
    mObj->selectedRowIndex = -1;
}

bool SelectionWindow::has_selected(FS::Path& path)
{
    if (mObj->selectedPath.empty())
        return false;

    path = mObj->selectedPath;
    mObj->selectedPath.clear();

    return true;
}

} // namespace LD