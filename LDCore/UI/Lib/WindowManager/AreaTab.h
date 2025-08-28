#pragma once

#include <Ludens/UI/UIWindowManager.h>

namespace LD {

struct AreaTab
{
    UITextWidget titleText; /// tab title text
    UIWindow client;        /// client window of this tab
    void (*onWindowResize)(UIWindow window, const Vec2& size);

    AreaTab(UIWindow client, UIWindow tabControl);

    void draw(ScreenRenderComponent renderer);
};

/// @brief class for the window manager to control tabs in an area
class AreaTabControl
{
public:
    void startup(UIContext ctx);
    void cleanup();

    void add_tab(UIWindow client);

    AreaTab* get_active_tab();

    void draw(ScreenRenderComponent renderer);

    /// @brief Invalidate the area rect.
    /// @param area The new area containing tabs and clients
    void invalidate_area(const Rect& area);

private:
    UIWindow mWindow;            /// tab control window
    AreaTab* mActiveTab;         /// focused tab
    std::vector<AreaTab*> mTabs; /// ordered tabs
};

} // namespace LD