#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>

namespace LD {

static bool get_directory_contents_with_filter(const FS::Path& directory, Vector<FS::Path>& contents, const char* extFilter)
{
    std::string err;

    if (!FS::get_directory_content(directory, contents, err))
        return false;

    FS::filter_files_by_extension(contents, extFilter);
    return true;
}

struct SelectionWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    RImage editorIconAtlas{};
    EditorTheme theme;
    std::string extensionFilter;
    Vector<FS::Path> directoryContents;
    FS::Path directoryPath;
    FS::Path selectedPath;
    int selectedRowIndex = -1;

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_SELECTION; }
    virtual void on_imgui(float delta) override;
    void top_bar();
    void bottom_bar();
    bool row(int idx);
};

void SelectionWindowObj::on_imgui(float delta)
{
    theme = ctx.get_theme();
    selectedPath.clear();

    ui_push_window(root);

    top_bar();

    // TODO: is this too frequent for calling Filesystem APIs?
    get_directory_contents_with_filter(directoryPath, directoryContents, extensionFilter.c_str());

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    ui_push_scroll(theme.get_ui_theme().get_surface_color());
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
}

void SelectionWindowObj::top_bar()
{
    float fontSize = theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    MouseButton btn;
    Rect iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::ArrowUpward);
    ui_push_image(editorIconAtlas, fontSize * 1.2f, fontSize * 1.2f, 0xFFFFFFFF, &iconRect);
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        directoryPath = directoryPath.parent_path();
        selectedRowIndex = -1;
    }
    ui_pop();

    std::string text = "Path: ";
    text += directoryPath.string();
    ui_push_text(text.c_str());
    ui_pop();

    ui_pop();
}

void SelectionWindowObj::bottom_bar()
{
    float pad = theme.get_padding();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childAlignX = UI_ALIGN_END;
    layoutI.childPadding = {pad, pad, pad, pad};
    layoutI.childGap = pad;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    bool isSelected;
    ui_push_button("select", isSelected);
    if (isSelected)
    {
        LD_ASSERT(0 <= selectedRowIndex && selectedRowIndex <= (int)directoryContents.size());
        selectedPath = directoryContents[selectedRowIndex];
    }
    ui_pop();

    bool isCancelled;
    ui_push_button("cancel", isCancelled);
    ui_pop();

    ui_pop();

    if (isSelected || isCancelled)
        mShouldClose = true;
}

bool SelectionWindowObj::row(int idx)
{
    bool isSelected = false;

    LD_ASSERT(0 <= idx && idx < directoryContents.size());

    MouseButton btn;
    EditorTheme edTheme = ctx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    float fontSize = edTheme.get_font_size();
    const FS::Path& itemPath = directoryContents[idx];
    float rowHeight = fontSize * 1.2f;
    bool isDirectory = FS::is_directory(itemPath);

    Color panelColor = idx == selectedRowIndex ? uiTheme.get_selection_color() : Color(0); // TODO:
    UILayoutInfo layoutI{};
    layoutI.sizeY = UISize::fixed(rowHeight);
    layoutI.sizeX = UISize::grow();
    layoutI.childAxis = UI_AXIS_X;
    ui_push_panel(&panelColor);
    ui_top_layout(layoutI);

    Rect iconRect = EditorIconAtlas::get_icon_rect(isDirectory ? EditorIcon::Folder : EditorIcon::Description);
    ui_push_image(editorIconAtlas, rowHeight, rowHeight, 0xFFFFFFFF, &iconRect);
    ui_pop();

    std::string fileString = itemPath.filename().string();
    ui_push_text(fileString.c_str());
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
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
    auto* obj = heap_new<SelectionWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), obj->ctx.make_vbox_layout(), {}, nullptr);
    obj->root.set_color(obj->ctx.get_theme().get_ui_theme().get_surface_color());
    obj->root.hide();
    obj->editorIconAtlas = obj->ctx.get_editor_icon_atlas();

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

    mObj->root.show();
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