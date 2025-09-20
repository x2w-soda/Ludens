#include "ComponentMenu.h"

#define OPT_ADD_SCRIPT 0
#define OPT_ADD_CHILD 1

namespace LD {

void ComponentMenu::startup(UIContext ctx, EditorTheme theme)
{
    UIDropdownWindowInfo dropdownWI{};
    dropdownWI.callback = &ComponentMenu::on_option;
    dropdownWI.context = ctx;
    dropdownWI.theme = theme;
    dropdownWI.user = this;
    mDropdown = UIDropdownWindow::create(dropdownWI);

    mDropdown.add_option("Add script", OPT_ADD_SCRIPT);
    mDropdown.add_option("Add child", OPT_ADD_CHILD);
}

void ComponentMenu::cleanup()
{
    UIDropdownWindow::destroy(mDropdown);
}

void ComponentMenu::show(const Vec2& pos)
{
    mDropdown.set_pos(pos);
    mDropdown.show();
}

void ComponentMenu::hide()
{
    mDropdown.hide();
}

void ComponentMenu::draw(ScreenRenderComponent renderer)
{
    mDropdown.draw(renderer);
}

bool ComponentMenu::on_option(int option, const Rect& optionRect, void* user)
{
    switch (option)
    {
    case OPT_ADD_SCRIPT:
        break;
    case OPT_ADD_CHILD:
        break;
    default:
        return false;
    }

    return true;
}

} // namespace LD