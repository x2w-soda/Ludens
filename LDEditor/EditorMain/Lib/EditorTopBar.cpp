#include "EditorTopBar.h"
#include "EditorUI.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorWidget/UIDropdownWindow.h>
#include <array>
#include <cstddef>
#include <cstdio>

#define FILE_OPTION_NEW_SCENE 0
#define FILE_OPTION_OPEN_SCENE 1
#define FILE_OPTION_SAVE_SCENE 2
#define FILE_OPTION_NEW_PROJECT 3
#define FILE_OPTION_OPEN_PROJECT 4

#define ABOUT_OPTION_VERSION 0

namespace LD {

struct MenuOption
{
    int index;
    const char* name;
};

/// @brief A menu in the editor topbar.
class TopBarMenu
{
public:
    /// @brief Create top bar menu.
    static TopBarMenu* create(EditorTopBar* bar, UINode node, EditorUI* editorUI, EditorTheme editorTheme, const char* cstr);

    /// @brief Destroy top bar menu.
    static void destroy(TopBarMenu* opt);

    /// @brief Hide the dropdown window.
    void hide_dropdown();

    /// @brief Call this once to initialize menu content.
    void set_content(size_t optionCount, const MenuOption* options, UIDropdownWindowCallback callback)
    {
        mDropdown.set_callback(callback);

        for (size_t i = 0; i < optionCount; i++)
            mDropdown.add_option(options[i].name, options[i].index);
    }

    EditorUI* get_editor_ui()
    {
        return mEditorUI;
    }

    static void on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
    {
        auto& self = *(TopBarMenu*)widget.get_user();
        UIWindow dropdown = self.mDropdown.get_native();

        if (event != UI_MOUSE_DOWN)
            return;

        if (dropdown.is_hidden())
        {
            UIWindow topbar = self.mBar->get_handle();
            float x = widget.get_pos().x;
            float y = topbar.get_size().y;
            self.mBar->set_active_menu(&self);

            Vec2 windowPos(x, y);
            dropdown.set_pos(windowPos);
            dropdown.raise();
            dropdown.show();
        }
        else
        {
            dropdown.hide();
        }
    }

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer)
    {
        TopBarMenu& self = *(TopBarMenu*)widget.get_user();

        self.mDropdown.get_native().draw(renderer);
    }

private:
    EditorUI* mEditorUI;        /// editor UI instance
    EditorTopBar* mBar;         /// editor top bar
    UIPanelWidget mPanel;       /// menu panel
    UITextWidget mText;         /// menu text on top of panel
    UIDropdownWindow mDropdown; /// menu dropdown window
};

TopBarMenu* TopBarMenu::create(EditorTopBar* bar, UINode node, EditorUI* editorUI, EditorTheme editorTheme, const char* cstr)
{
    auto menu = heap_new<TopBarMenu>(MEMORY_USAGE_UI);
    menu->mBar = bar;
    menu->mEditorUI = editorUI;

    float fontSize;
    editorTheme.get_font_size(fontSize);

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::grow();

    UIPanelWidgetInfo panelWI{};
    menu->mPanel = node.add_panel(layoutI, panelWI, menu);
    menu->mPanel.set_on_mouse(&TopBarMenu::on_mouse);
    menu->mPanel.set_on_draw(&TopBarMenu::on_draw);

    UITextWidgetInfo textWI{};
    textWI.cstr = cstr;
    textWI.fontSize = fontSize;
    textWI.hoverHL = true;
    menu->mText = menu->mPanel.node().add_text({}, textWI, menu);
    menu->mText.set_on_mouse(&TopBarMenu::on_mouse);

    UIContext ctx(node.get_context());

    UIDropdownWindowInfo dropdownWI{};
    dropdownWI.callback = nullptr;
    dropdownWI.context = ctx;
    dropdownWI.theme = editorTheme;
    dropdownWI.user = menu;
    menu->mDropdown = UIDropdownWindow::create(dropdownWI);
    menu->mDropdown.get_native().hide();

    return menu;
}

void TopBarMenu::destroy(TopBarMenu* opt)
{
    heap_delete<TopBarMenu>(opt);
}

void TopBarMenu::hide_dropdown()
{
    mDropdown.get_native().hide();
}

void EditorTopBar::startup(const EditorTopBarInfo& info)
{
    UIContext ctx = info.context;
    mTopBarHeight = info.barHeight;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.childGap = 6.0f;
    layoutI.childPadding = {.left = 6.0f};
    layoutI.sizeX = UISize::fixed(info.screenSize.x);
    layoutI.sizeY = UISize::fixed(mTopBarHeight);
    UIWindowInfo windowI{};
    windowI.name = "EditorTopBar";
    windowI.defaultMouseControls = false;

    mRoot = ctx.add_window(layoutI, windowI, nullptr);
    mRoot.set_pos(Vec2(0.0f, 0.0f));
    mRoot.set_user(this);

    std::array<MenuOption, 5> fileMenuOptions = {
        MenuOption(FILE_OPTION_NEW_SCENE, "New Scene"),
        MenuOption(FILE_OPTION_OPEN_SCENE, "Open Scene"),
        MenuOption(FILE_OPTION_SAVE_SCENE, "Save Scene"),
        MenuOption(FILE_OPTION_NEW_PROJECT, "New Project"),
        MenuOption(FILE_OPTION_OPEN_PROJECT, "Open Project"),
    };
    mFileMenu = TopBarMenu::create(this, mRoot.node(), info.editorUI, info.editorTheme, "File");
    mFileMenu->set_content(fileMenuOptions.size(), fileMenuOptions.data(), &EditorTopBar::on_file_menu_option);

    std::array<MenuOption, 1> aboutMenuOptions = {
        MenuOption(FILE_OPTION_NEW_SCENE, "Version"),
    };
    mAboutMenu = TopBarMenu::create(this, mRoot.node(), info.editorUI, info.editorTheme, "About");
    mAboutMenu->set_content(aboutMenuOptions.size(), aboutMenuOptions.data(), &EditorTopBar::on_about_menu_option);
}

void EditorTopBar::cleanup()
{
    TopBarMenu::destroy(mAboutMenu);
    TopBarMenu::destroy(mFileMenu);
}

void EditorTopBar::set_active_menu(TopBarMenu* menu)
{
    if (menu != mFileMenu)
        mFileMenu->hide_dropdown();
    if (menu != mAboutMenu)
        mAboutMenu->hide_dropdown();
}

UIWindow EditorTopBar::get_handle()
{
    return mRoot;
}

bool EditorTopBar::on_file_menu_option(int opt, const Rect& rect, void* user)
{
    switch (opt)
    {
    case FILE_OPTION_NEW_SCENE:
        break;
    case FILE_OPTION_OPEN_SCENE:
        break;
    case FILE_OPTION_SAVE_SCENE:
        break;
    case FILE_OPTION_NEW_PROJECT:
        break;
    case FILE_OPTION_OPEN_PROJECT:
        break;
    }

    return true;
}

bool EditorTopBar::on_about_menu_option(int opt, const Rect& rect, void* user)
{
    TopBarMenu& aboutMenu = *(TopBarMenu*)user;
    EditorUI* editorUI = aboutMenu.get_editor_ui();

    switch (opt)
    {
    case ABOUT_OPTION_VERSION:
        editorUI->show_version_window();
        break;
    }

    return true;
}

} // namespace LD