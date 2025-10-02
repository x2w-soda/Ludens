#include <Ludens/Header/Impulse.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorWidget/UISelectWindow.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace LD {

struct UISelectWindowObj;

/// @brief Select window top bar.
struct TopBar
{
    void startup(UISelectWindowObj* obj, UINode parent);
    void display(const FS::Path& currentDir);

    UISelectWindowObj* windowObj;
    UIPanelWidget rootW;
    UIButtonWidget upwardsW; // TODO: icon
    UITextWidget pathTextW;
};

/// @brief Select window bottom bar.
struct BottomBar
{
    void startup(UISelectWindowObj* obj, UINode parent);

    UISelectWindowObj* windowObj;
    UIPanelWidget rootW;
    UIButtonWidget selectBtnW;
    UIButtonWidget cancelBtnW;
};

/// @brief Selection window implementation.
struct UISelectWindowObj
{
    /// @brief A single directory or file.
    struct Item
    {
        UIPanelWidget panelW;
        UIImageWidget iconW;
        UITextWidget textW;
        FS::Path path;
        Color panelColor;
        int index;
        Impulse enterDirectoryRequest;
        Impulse selectRequest;
        bool isDirectory;

        void display(const FS::Path& itemPath);
    };

    EditorContext editorCtx;
    EditorTheme theme;
    UIWindow root;
    FS::Path currentDir;
    FS::Path selectedPath;
    int selectedIndex = -1;
    TopBar topBar;
    BottomBar bottomBar;
    UITextWidget contentTextW;
    UIScrollWidget containerW;
    std::vector<Item*> items;
    std::string extFilter;
    void (*onSelect)(const FS::Path& path, void* user) = nullptr;
    void (*onCancel)(void* user) = nullptr;
    void* user = nullptr;

    void display(const FS::Path& path);
    void display_parent();
    void display_child(int index);
    void select_item(Item* item);
    void unselect_item();
    Item* get_or_create_item(int idx, const FS::Path& path);

    static void on_update(UIWidget widget, float delta);
};

void TopBar::startup(UISelectWindowObj* obj, UINode parent)
{
    windowObj = obj;
    float fontSize = obj->theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    UIPanelWidgetInfo panelWI{};
    panelWI.color = {};
    rootW = parent.add_panel(layoutI, panelWI, nullptr);

    layoutI.sizeX = UISize::fixed(30);
    layoutI.sizeY = UISize::grow();
    UIButtonWidgetInfo buttonWI{};
    buttonWI.text = "P";
    buttonWI.textColor = 0xFFFFFFFF;
    buttonWI.transparentBG = false;
    buttonWI.on_press = [](UIButtonWidget w, MouseButton btn, void* user) {
        TopBar& self = *(TopBar*)user;
        self.windowObj->display_parent();
    };
    upwardsW = rootW.node().add_button(layoutI, buttonWI, this);

    UITextWidgetInfo textWI{};
    textWI.fontSize = fontSize;
    textWI.hoverHL = false;
    pathTextW = rootW.node().add_text({}, textWI, nullptr);
}

void TopBar::display(const FS::Path& currentDir)
{
    std::string text = "Path: ";
    text += currentDir.string();
    pathTextW.set_text(text.c_str());
}

void BottomBar::startup(UISelectWindowObj* obj, UINode parent)
{
    windowObj = obj;

    float fontSize = obj->theme.get_font_size();
    float pad = obj->theme.get_padding();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childAlignX = UI_ALIGN_END;
    layoutI.childPadding = {pad, pad, pad, pad};
    layoutI.childGap = pad;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    UIPanelWidgetInfo panelWI{};
    panelWI.color = {};
    rootW = parent.add_panel(layoutI, panelWI, nullptr);

    layoutI.sizeX = UISize::fixed(50);
    layoutI.sizeY = UISize::fixed(fontSize * 1.2f);
    UIButtonWidgetInfo buttonWI{};
    buttonWI.text = "select";
    buttonWI.textColor = 0xFFFFFFFF;
    buttonWI.transparentBG = false;
    buttonWI.on_press = [](UIButtonWidget w, MouseButton btn, void* user) {
        BottomBar& self = *(BottomBar*)user;
        UISelectWindowObj* obj = self.windowObj;
        if (obj->onSelect)
            obj->onSelect(obj->selectedPath, obj->user);
    };
    selectBtnW = rootW.node().add_button(layoutI, buttonWI, this);

    buttonWI.text = "cancel";
    buttonWI.on_press = [](UIButtonWidget w, MouseButton btn, void* user) {
        BottomBar& self = *(BottomBar*)user;
        UISelectWindowObj* obj = self.windowObj;
        if (obj->onCancel)
            obj->onCancel(obj->user);
    };
    cancelBtnW = rootW.node().add_button(layoutI, buttonWI, this);
}

void UISelectWindowObj::Item::display(const FS::Path& itemPath)
{
    if (itemPath.empty())
    {
        panelW.hide();
        return;
    }

    isDirectory = FS::is_directory(itemPath);
    std::string fileNameString = itemPath.filename().string();
    path = itemPath;
    textW.set_text(fileNameString.c_str());
    iconW.set_image_rect(EditorIconAtlas::get_icon_rect(isDirectory ? EditorIcon::Folder : EditorIcon::Description));
    panelW.show();
}

void UISelectWindowObj::display(const FS::Path& directory)
{
    std::vector<FS::Path> contents;
    std::string err;
    if (!FS::get_directory_content(directory, contents, err))
        return;

    FS::filter_files_by_extension(contents, extFilter.c_str());

    currentDir = directory;
    topBar.display(currentDir);

    int itemCount = (int)contents.size();
    for (int i = 0; i < itemCount; i++)
    {
        Item* item = get_or_create_item(i, contents[i]);
        item->display(contents[i]);
    }

    int rowCount = (int)items.size();
    for (int i = itemCount; i < rowCount; i++)
    {
        Item* item = get_or_create_item(i, {});
        item->display({});
    }
}

void UISelectWindowObj::display_parent()
{
    display(currentDir.parent_path());
    unselect_item();
}

void UISelectWindowObj::display_child(int index)
{
    LD_ASSERT(0 <= index && index < (int)items.size());

    Item* item = get_or_create_item(index, items[index]->path);
    display(item->path);
}

void UISelectWindowObj::select_item(Item* item)
{
    bool validItem;

    if (!item || (validItem = 0 <= item->index && item->index < (int)items.size()))
    {
        unselect_item();

        if (!item || !validItem)
            return;
    }

    selectedPath = item->path;
    selectedIndex = item->index;

    UITheme uiTheme = theme.get_ui_theme();
    item->panelW.set_panel_color(uiTheme.get_selection_color());
}

void UISelectWindowObj::unselect_item()
{
    if (0 <= selectedIndex && selectedIndex < (int)items.size())
    {
        Item* oldSelection = items[selectedIndex];
        oldSelection->panelW.set_panel_color(0);
    }

    selectedIndex = -1;
}

UISelectWindowObj::Item* UISelectWindowObj::get_or_create_item(int idx, const FS::Path& path)
{
    if (idx >= (int)items.size())
    {
        int oldSize = (int)items.size();
        items.resize(idx + 1);
        for (int i = oldSize; i < (int)items.size(); i++)
            items[i] = nullptr;
    }

    if (!items[idx])
    {
        float fontSize = theme.get_font_size();
        float rowHeight = fontSize * 1.2f;

        Item* item = items[idx] = heap_new<Item>(MEMORY_USAGE_UI);
        item->index = idx;
        item->selectRequest.set(false);
        item->enterDirectoryRequest.set(false);
        item->isDirectory = FS::is_directory(path);
        UILayoutInfo layoutI{};
        layoutI.sizeY = UISize::fixed(rowHeight);
        layoutI.sizeX = UISize::grow();
        layoutI.childAxis = UI_AXIS_X;
        UIPanelWidgetInfo panelWI{};
        item->panelW = containerW.node().add_panel(layoutI, panelWI, item);

        layoutI.sizeY = UISize::fixed(rowHeight);
        layoutI.sizeX = UISize::fixed(rowHeight);
        Rect rect = EditorIconAtlas::get_icon_rect(item->isDirectory ? EditorIcon::Folder : EditorIcon::Description);
        UIImageWidgetInfo imageWI{};
        imageWI.image = editorCtx.get_editor_icon_atlas();
        imageWI.rect = &rect;
        item->iconW = item->panelW.node().add_image(layoutI, imageWI, item);

        std::string pathString = path.string();
        UITextWidgetInfo textWI{};
        textWI.fontSize = fontSize;
        textWI.cstr = pathString.c_str();
        textWI.hoverHL = true;
        item->textW = item->panelW.node().add_text({}, textWI, item);
        item->textW.set_on_mouse([](UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event) {
            Item* item = (Item*)widget.get_user();

            if (btn != MOUSE_BUTTON_LEFT || event != UI_MOUSE_DOWN)
                return;

            if (item->isDirectory)
            {
                item->enterDirectoryRequest.set(true);
            }
            else
            {
                item->selectRequest.set(true);
            }
        });
    }

    return items[idx];
}

void UISelectWindowObj::on_update(UIWidget widget, float delta)
{
    auto& self = *(UISelectWindowObj*)widget.get_user();

    Item* toSelect = nullptr;
    Item* toEnter = nullptr;

    for (Item* item : self.items)
    {
        if (item->selectRequest.read())
            toSelect = item;

        if (item->enterDirectoryRequest.read())
            toEnter = item;
    }

    if (toSelect)
        self.select_item(toSelect);

    if (toEnter)
    {
        self.display_child(toEnter->index);
        self.select_item(nullptr);
    }
}

//
// Public API
//

UISelectWindow UISelectWindow::create(const UISelectWindowInfo& info)
{
    UIContext ctx = info.context;
    auto* obj = heap_new<UISelectWindowObj>(MEMORY_USAGE_UI);
    obj->editorCtx = info.editorCtx;
    obj->theme = obj->editorCtx.get_theme();

    float fontSize = obj->theme.get_font_size();
    float pad = obj->theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childPadding.left = pad;
    layoutI.childPadding.right = pad;
    layoutI.sizeX = UISize::fixed(600);
    layoutI.sizeY = UISize::fixed(400);
    UIWindowInfo windowI{};
    windowI.name = "UISelectWindow";
    obj->root = ctx.add_window(layoutI, windowI, obj);
    obj->root.set_on_update(&UISelectWindowObj::on_update);
    UINode rootNode = obj->root.node();

    obj->topBar.startup(obj, rootNode);

    {
        UITextWidgetInfo textWI{};
        textWI.fontSize = fontSize;
        textWI.cstr = "Content: ";
        obj->contentTextW = rootNode.add_text({}, textWI, obj);

        // TODO: experimental
        UIScrollWidgetInfo scrollWI{};
        scrollWI.hasScrollBar = true;
        layoutI.sizeX = UISize::grow();
        layoutI.sizeY = UISize::grow();
        obj->containerW = rootNode.add_scroll(layoutI, scrollWI, obj);
        obj->containerW.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
            UISelectWindowObj* obj = (UISelectWindowObj*)widget.get_user();
            Color color = obj->theme.get_ui_theme().get_field_color();
            renderer.draw_rect(widget.get_rect(), color);
        });
    }

    obj->bottomBar.startup(obj, rootNode);
    obj->display(info.directory);
    obj->root.layout();

    return UISelectWindow(obj);
}

void UISelectWindow::destroy(UISelectWindow window)
{
    UISelectWindowObj* obj = window.unwrap();
    UIContext ctx(obj->root.node().get_context());

    ctx.remove_window(obj->root);

    for (auto* item : obj->items)
        heap_delete<UISelectWindowObj::Item>(item);

    heap_delete<UISelectWindowObj>(obj);
}

UIWindow UISelectWindow::get_handle()
{
    return mObj->root;
}

void UISelectWindow::set_directory(const FS::Path& directory)
{
    mObj->currentDir = directory;
    mObj->display(mObj->currentDir);
}

void UISelectWindow::set_extension_filter(const char* ext)
{
    if (!ext)
    {
        mObj->extFilter.clear();
        return;
    }

    while (ext[0] == '.')
        ext++;

    mObj->extFilter = std::string(ext);
    mObj->display(mObj->currentDir);
}

void UISelectWindow::set_on_select(void (*onSelect)(const FS::Path& path, void* user), void* user)
{
    mObj->onSelect = onSelect;
    mObj->user = user;
}

void UISelectWindow::set_on_cancel(void (*onCancel)(void* user))
{
    mObj->onCancel = onCancel;
}

} // namespace LD