#include <array>

#include "ComponentMenu.h"

#define COMPONENT_OPTION_ADD_SCRIPT 0
#define COMPONENT_OPTION_ADD_CHILD 1

namespace LD {
#if 0

struct MenuOption
{
    int index;
    const char *name;
};

void ComponentMenu::startup(const ComponentMenuInfo& info)
{
    LD_ASSERT(info.layer);

    mInfo = info;

    UIDropdownWindowInfo dropdownWI{};
    dropdownWI.callback = &ComponentMenu::on_option;
    dropdownWI.context = mInfo.ctx;
    dropdownWI.theme = mInfo.theme;
    dropdownWI.user = this;
    dropdownWI.layer = info.layer;
    mDropdown = UIDropdownWindow::create(dropdownWI);

    std::array<MenuOption, 2> componentMenuOptions = {
        MenuOption(COMPONENT_OPTION_ADD_SCRIPT, "Add script"),
        MenuOption(COMPONENT_OPTION_ADD_CHILD, "Add child"),
    };

    for (auto& menuOpt : componentMenuOptions) {
        mDropdown.add_option(menuOpt.name, menuOpt.index);
    }
}

void ComponentMenu::cleanup()
{
    UIDropdownWindow::destroy(mDropdown);
}

void ComponentMenu::show(const Vec2& pos, CUID cuid)
{
    mCUID = cuid;
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
    ComponentMenu& self = *(ComponentMenu*)user;

    switch (option)
    {
    case COMPONENT_OPTION_ADD_SCRIPT:
        if (self.mInfo.onOptionAddScript)
            self.mInfo.onOptionAddScript(self.mCUID, self.mInfo.user);
        break;
    case COMPONENT_OPTION_ADD_CHILD:
        break;
    default:
        return false;
    }

    return true;
}

#endif
} // namespace LD
