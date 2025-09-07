#pragma once

#include <Ludens/UI/UIWindow.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct TopBarMenu;

/// @brief Editor top bar menu UI.
class EditorTopBar
{
public:
    /// @brief In-place startup.
    void startup(UIWindow root, EditorTheme theme);

    /// @brief In-place cleanup.
    void cleanup();

    /// @brief Get bar height.
    float get_height();

    /// @brief Only one menu window can be active in the top bar.
    void set_active_menu(TopBarMenu* menu);

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