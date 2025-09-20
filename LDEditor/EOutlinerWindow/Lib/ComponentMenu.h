#pragma once

#include <LudensEditor/EditorContext/EditorSettings.h>
#include <LudensEditor/EditorWidget/UIDropdownWindow.h>

namespace LD {

/// @brief Dropdown menu for options applicable to
///        components inside the Outliner window.
class ComponentMenu
{
public:
    void startup(UIContext ctx, EditorTheme theme);
    void cleanup();

    void show(const Vec2& pos);
    void hide();

    void draw(ScreenRenderComponent renderer);

private:
    static bool on_option(int option, const Rect& optionRect, void* user);

    UIDropdownWindow mDropdown;
};

} // namespace LD