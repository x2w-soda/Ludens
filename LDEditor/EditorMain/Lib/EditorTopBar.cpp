#include "EditorTopBar.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorWidget/UIDropdownWindow.h>
#include <array>
#include <cstddef>
#include <cstdio>

namespace LD {

/// @brief A menu in the editor topbar.
class TopBarMenu
{
public:
    /// @brief Create top bar menu.
    static TopBarMenu* create(EditorTopBar* bar, UINode node, EditorTheme theme, const char* cstr);

    /// @brief Destroy top bar menu.
    static void destroy(TopBarMenu* opt);

    /// @brief Call this once to initialize menu content.
    void set_content(size_t optionCount, const char** options, UIDropdownWindowCallback callback)
    {
        mDropdown.set_callback(callback);

        for (size_t i = 0; i < optionCount; i++)
            mDropdown.add_option(options[i]);
    }

    static void on_mouse_down(UIWidget widget, const Vec2& pos, MouseButton btn)
    {
        auto& self = *(TopBarMenu*)widget.get_user();

        float x = widget.get_pos().x;
        float y = self.mBar->get_height();

        Vec2 windowPos(x, y);
        UIWindow dropdown = self.mDropdown.get_native();
        dropdown.set_pos(windowPos);
        dropdown.raise();
        dropdown.show();
    }

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer)
    {
        TopBarMenu& self = *(TopBarMenu*)widget.get_user();

        self.mDropdown.get_native().draw(renderer);
    }

private:
    EditorTopBar* mBar;         /// editor top bar
    UIPanelWidget mPanel;       /// menu panel
    UITextWidget mText;         /// menu text on top of panel
    UIDropdownWindow mDropdown; /// menu dropdown window
};

TopBarMenu* TopBarMenu::create(EditorTopBar* bar, UINode node, EditorTheme theme, const char* cstr)
{
    auto menu = heap_new<TopBarMenu>(MEMORY_USAGE_UI);
    menu->mBar = bar;

    float fontSize;
    theme.get_font_size(fontSize);

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::grow();

    UIPanelWidgetInfo panelWI{};
    menu->mPanel = node.add_panel(layoutI, panelWI, menu);
    menu->mPanel.set_on_mouse_down(&TopBarMenu::on_mouse_down);
    menu->mPanel.set_on_draw(&TopBarMenu::on_draw);

    UITextWidgetInfo textWI{};
    textWI.cstr = cstr;
    textWI.fontSize = fontSize;
    textWI.hoverHL = true;
    menu->mText = menu->mPanel.node().add_text({}, textWI, menu);
    menu->mText.set_on_mouse_down(&TopBarMenu::on_mouse_down);

    UIContext ctx(node.get_context());

    UIDropdownWindowInfo dropdownWI{};
    dropdownWI.callback = nullptr;
    dropdownWI.context = ctx;
    dropdownWI.theme = theme;
    dropdownWI.user = menu;
    menu->mDropdown = UIDropdownWindow::create(dropdownWI);
    menu->mDropdown.get_native().hide();

    return menu;
}

void TopBarMenu::destroy(TopBarMenu* opt)
{
    heap_delete<TopBarMenu>(opt);
}

void EditorTopBar::startup(UIWindow root, EditorTheme theme)
{
    mRoot = root;
    mRoot.set_user(this);

    std::array<const char*, 3> fileMenuOptions = {
        "New Scene",
        "Foo",
        "Bar",
    };
    mFileMenu = TopBarMenu::create(this, mRoot.node(), theme, "File");
    mFileMenu->set_content(fileMenuOptions.size(), fileMenuOptions.data(), &EditorTopBar::on_file_menu_option);

    mAboutMenu = TopBarMenu::create(this, mRoot.node(), theme, "About");
}

void EditorTopBar::cleanup()
{
    TopBarMenu::destroy(mAboutMenu);
    TopBarMenu::destroy(mFileMenu);
}

float EditorTopBar::get_height()
{
    return mRoot.get_size().y;
}

void EditorTopBar::on_file_menu_option(int opt, const Rect& rect, void* user)
{
    printf("on_file_menu_option: %d\n", opt);
}

void EditorTopBar::on_about_menu_option(int opt, const Rect& rect, void* user)
{
    // TODO:
}

} // namespace LD