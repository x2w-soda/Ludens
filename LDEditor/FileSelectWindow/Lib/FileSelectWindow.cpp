#include <Ludens/DSA/StringUtil.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/FileSelectWindow/FileSelectWindow.h>

namespace LD {

static bool get_directory_contents_with_filter(const FS::Path& directory, Vector<FS::Path>& contents, const char* extFilter)
{
    String err;

    if (!FS::get_directory_content(directory, contents, false, err))
        return false;

    FS::filter_files_by_extension(contents, extFilter);
    return true;
}

struct FileSelectWindowObj : EditorWindowObj
{
    RImage editorIconAtlas{};
    EditorTheme theme;
    String extensionFilter;
    Vector<FS::Path> directoryContents;
    FS::Path directoryPath;
    FS::Path selectedPath;
    int selectedRowIndex = -1;

    FileSelectWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
        editorIconAtlas = ctx.get_editor_icon_atlas();
    }

    void update(float delta);
    void top_bar();
    void bottom_bar();
    bool row(int idx);
};

void FileSelectWindowObj::update(float delta)
{
    theme = ctx.get_theme();
    selectedPath.clear();

    begin_update_window();

    ui_top_layout(theme.make_vbox_layout_fixed(rootRect.get_size()));
    ui_window_set_color(theme.get_ui_theme().get_surface_color());

    top_bar();

    // TODO: is this too frequent for calling Filesystem APIs?
    get_directory_contents_with_filter(directoryPath, directoryContents, extensionFilter.c_str());

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childGap = 2.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    auto* scrollData = (UIScrollData*)ui_push_scroll(nullptr).get_data();
    scrollData->bgColor = theme.get_ui_theme().get_surface_color();
    ui_top_layout(layoutI);

    int contentCount = (int)directoryContents.size();

    for (int idx = 0; idx < contentCount; idx++)
    {
        if (row(idx))
            selectedRowIndex = idx;
    }
    ui_pop();

    bottom_bar();

    end_update_window();
}

void FileSelectWindowObj::top_bar()
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
    auto* imageData = (UIImageData*)ui_push_image(nullptr, fontSize * 1.2f, fontSize * 1.2f).get_data();
    imageData->image = editorIconAtlas;
    imageData->rect = EditorIconAtlas::get_icon_rect(EDITOR_ICON_ARROW_UP);
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
    {
        directoryPath = directoryPath.parent_path();
        selectedRowIndex = -1;
    }
    ui_pop();

    String text = "Path: ";
    text += directoryPath.string();
    ui_push_text(nullptr, text.c_str());
    ui_pop();

    ui_pop();
}

void FileSelectWindowObj::bottom_bar()
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
        shouldClose = true;
}

bool FileSelectWindowObj::row(int idx)
{
    bool isSelected = false;

    LD_ASSERT(0 <= idx && idx < directoryContents.size());

    Vec2 mousePos;
    MouseValue mouseVal;
    EditorTheme edTheme = ctx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    float fontSize = edTheme.get_font_size();
    const FS::Path& itemPath = directoryContents[idx];
    float rowHeight = fontSize * 1.2f;
    bool isDirectory = FS::is_directory(itemPath);
    UIImageData* imageS;
    UIPanelData* panelS;

    Color panelColor = idx == selectedRowIndex ? uiTheme.get_selection_color() : Color(0); // TODO:
    UILayoutInfo layoutI{};
    layoutI.sizeY = UISize::fixed(rowHeight);
    layoutI.sizeX = UISize::grow();
    layoutI.childAxis = UI_AXIS_X;
    panelS = (UIPanelData*)ui_push_panel(nullptr).get_data();
    panelS->color = panelColor;
    ui_top_layout(layoutI);

    imageS = (UIImageData*)ui_push_image(nullptr, rowHeight, rowHeight).get_data();
    imageS->image = editorIconAtlas;
    imageS->rect = EditorIconAtlas::get_icon_rect(isDirectory ? EDITOR_ICON_FOLDER : EDITOR_ICON_FILE);
    ui_pop();

    String fileString = to_string(itemPath.filename().string());
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

EditorWindow FileSelectWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<FileSelectWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow(obj);
}

void FileSelectWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<FileSelectWindowObj*>(window.unwrap());

    heap_delete<FileSelectWindowObj>(obj);
}

void FileSelectWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<FileSelectWindowObj*>(base);

    obj->update(tick.delta);
}

void FileSelectWindow::show(const FS::Path& directoryPath, const char* extensionFilter)
{
    mObj->directoryPath = directoryPath;
    mObj->directoryContents.clear();
    mObj->extensionFilter = String(extensionFilter);
    mObj->selectedPath.clear();
    mObj->selectedRowIndex = -1;
}

bool FileSelectWindow::has_selected(FS::Path& path)
{
    if (mObj->selectedPath.empty())
        return false;

    path = mObj->selectedPath;
    mObj->selectedPath.clear();

    return true;
}

} // namespace LD