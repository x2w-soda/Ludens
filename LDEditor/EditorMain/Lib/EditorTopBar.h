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

private:
    static void on_file_menu_option(int opt, const Rect& rect, void* user);
    static void on_about_menu_option(int opt, const Rect& rect, void* user);

private:
    UIWindow mRoot;
    TopBarMenu* mFileMenu;
    TopBarMenu* mAboutMenu;
    float mTopBarHeight;
};

} // namespace LD