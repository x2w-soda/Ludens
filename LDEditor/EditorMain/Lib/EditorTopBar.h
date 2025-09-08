#pragma once

#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindow.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct TopBarMenu;

struct EditorTopBarInfo
{
    UIContext context;
    EditorTheme theme;
    Vec2 screenSize;
    float barHeight;
};

/// @brief Editor top bar menu UI.
class EditorTopBar
{
public:
    /// @brief In-place startup.
    void startup(const EditorTopBarInfo& info);

    /// @brief In-place cleanup.
    void cleanup();

    /// @brief Only one menu window can be active in the top bar.
    void set_active_menu(TopBarMenu* menu);

    /// @brief Get window handle.
    UIWindow get_handle();

private:
    static bool on_file_menu_option(int opt, const Rect& rect, void* user);
    static bool on_about_menu_option(int opt, const Rect& rect, void* user);

private:
    UIWindow mRoot;
    TopBarMenu* mFileMenu;
    TopBarMenu* mAboutMenu;
    float mTopBarHeight;
};

} // namespace LD