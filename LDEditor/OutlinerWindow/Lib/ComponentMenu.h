#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <LudensEditor/EditorContext/EditorSettings.h>
#include <LudensEditor/EditorWidget/UIListMenuWidget.h>

namespace LD {
#if 0

struct ComponentMenuInfo
{
    UIContext ctx;
    EditorTheme theme;
    Hash32 layer;
    void (*onOptionAddScript)(CUID cuid, void* user);
    void* user;
};

/// @brief Dropdown menu for options applicable to
///        components inside the Outliner window.
class ComponentMenu
{
public:
    void startup(const ComponentMenuInfo& info);
    void cleanup();

    void show(const Vec2& pos, CUID cuid);
    void hide();

    void draw(ScreenRenderComponent renderer);

private:
    static bool on_option(int option, const Rect& optionRect, void* user);

    //UIDropdownWindow mDropdown;
    ComponentMenuInfo mInfo;
    CUID mCUID;
};

#endif
} // namespace LD