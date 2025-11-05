#include <Ludens/Header/Impulse.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorWidget/UISelectWindow.h>
#include <string>
#include <vector>

namespace LD {

static void eui_select_window_top_bar(EUISelectWindow* window);
static bool eui_select_window_bottom_bar(EUISelectWindow* window);
static bool eui_select_window_row(EUISelectWindow* window, EUISelectWindowRow* row);
static bool get_directory_contents_with_filter(const FS::Path& directory, std::vector<FS::Path>& contents, const char* extFilter);

bool eui_select_window(EUISelectWindow* window, FS::Path& selectedPath)
{
    if (!window->isActive)
        return false;

    if (window->isContentDirty)
    {
        bool ok = get_directory_contents_with_filter(window->directoryPath, window->directoryContents, window->extensionFilter);
        // TODO: error control flow
        window->isContentDirty = !ok;
    }

    UITheme uiTheme = window->theme.get_ui_theme();
    float fontSize = window->theme.get_font_size();
    float pad = window->theme.get_padding();

    ui_push_window(window->clientName, window->client);

    eui_select_window_top_bar(window);
    if (window->isContentDirty)
    {
        bool ok = get_directory_contents_with_filter(window->directoryPath, window->directoryContents, window->extensionFilter);
        // TODO: error control flow
        window->isContentDirty = !ok;
    }

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    ui_push_scroll(uiTheme.get_surface_color());
    ui_top_layout(layoutI);

    int contentCount = (int)window->directoryContents.size();
    window->rows.resize(contentCount);

    for (int idx = 0; idx < contentCount; idx++)
    {
        EUISelectWindowRow* row = window->rows.data() + idx;
        row->window = window;
        row->idx = idx;
        if (eui_select_window_row(window, row))
        {
            window->highlightedItemIndex = idx;
        }
    }
    ui_pop();

    bool hasSelected = eui_select_window_bottom_bar(window);
    if (window->highlightedItemIndex < 0)
        hasSelected = false;

    if (hasSelected)
    {
        int rowIdx = window->highlightedItemIndex;
        LD_ASSERT(0 <= rowIdx && rowIdx < (int)window->directoryContents.size());
        selectedPath = window->directoryContents[rowIdx];
    }

    ui_pop_window();

    return hasSelected;
}

static void eui_select_window_top_bar(EUISelectWindow* window)
{
    MouseButton btn;
    float fontSize = window->theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    Rect iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::ArrowUpward);
    ui_push_image(window->editorIconAtlas, fontSize * 1.2f, fontSize * 1.2f, &iconRect);
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        window->directoryPath = window->directoryPath.parent_path();
        window->highlightedItemIndex = -1;
        window->isContentDirty = true;
    }
    ui_pop();

    std::string text = "Path: ";
    text += window->directoryPath.string();
    ui_push_text(text.c_str());
    ui_pop();

    ui_pop();
}

bool eui_select_window_bottom_bar(EUISelectWindow* window)
{
    float pad = window->theme.get_padding();

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
    ui_pop();

    bool isCanceled;
    ui_push_button("cancel", isCanceled);
    ui_pop();

    ui_pop();

    window->isActive = !isSelected && !isCanceled;

    return isSelected;
}

bool eui_select_window_row(EUISelectWindow* window, EUISelectWindowRow* row)
{
    LD_ASSERT(0 <= row->idx && row->idx < window->directoryContents.size());

    MouseButton btn;
    float fontSize = window->theme.get_font_size();
    const FS::Path& itemPath = window->directoryContents[row->idx];
    float rowHeight = fontSize * 1.2f;
    bool isDirectory = FS::is_directory(itemPath);
    bool isHighlighted = false;

    UILayoutInfo layoutI{};
    layoutI.sizeY = UISize::fixed(rowHeight);
    layoutI.sizeX = UISize::grow();
    layoutI.childAxis = UI_AXIS_X;
    ui_push_panel();
    ui_top_layout(layoutI);
    ui_top_user(row);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* imUser) {
        EUISelectWindowRow* row = (EUISelectWindowRow*)imUser;
        EUISelectWindow* window = row->window;
        Color hlColor = window->theme.get_ui_theme().get_selection_color();

        if (window->highlightedItemIndex == row->idx)
            renderer.draw_rect(widget.get_rect(), hlColor);
    });

    Rect iconRect = EditorIconAtlas::get_icon_rect(isDirectory ? EditorIcon::Folder : EditorIcon::Description);
    ui_push_image(window->editorIconAtlas, rowHeight, rowHeight, &iconRect);
    ui_pop();

    std::string fileString = itemPath.filename().string();
    ui_push_text(fileString.c_str());
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        isHighlighted = true;

        if (isDirectory)
        {
            window->directoryPath = itemPath;
            window->isContentDirty = true;
        }
        else
        {
            // TODO:
        }
    }
    ui_pop();

    ui_pop();

    return isHighlighted;
}

static bool get_directory_contents_with_filter(const FS::Path& directory, std::vector<FS::Path>& contents, const char* extFilter)
{
    std::string err;

    if (!FS::get_directory_content(directory, contents, err))
        return false;

    FS::filter_files_by_extension(contents, extFilter);
    return true;
}

} // namespace LD