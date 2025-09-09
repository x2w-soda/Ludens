#pragma once

#include <Ludens/UI/UIWindowManager.h>

namespace LD {

/// @brief Tabs allow multiple clients to occupy the same area.
///        Only a single tab is active in an area at a time.
struct AreaTab
{
    UIPanelWidget panelW;                    /// tab panel widget
    UITextWidget titleTextW;                 /// tab title text widget
    UIWindow client;                         /// client window of this tab
    UIWMClientResizeCallback onClientResize; /// client window resize callback

    AreaTab(UIWindow client, UIWindow tabControl);
};

/// @brief class for the window manager to control tabs in an area
class AreaTabControl
{
public:
    void startup_as_leaf(UIContext ctx, const Rect& area);
    void startup_as_float(UIContext ctx, const Rect& area, float border);
    void cleanup();

    void add_tab(UIWindow client);

    AreaTab* get_active_tab();

    void draw(ScreenRenderComponent renderer);

    /// @brief Invalidate the area rect.
    /// @param area The new area containing tabs and clients
    void invalidate_area(const Rect& area);

private:
    void invalidate_float_area(const Rect& area);
    void invalidate_leaf_area(const Rect& area);

    static void on_float_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    static void on_float_draw(UIWidget widget, ScreenRenderComponent renderer);

private:
    bool mIsFloat;               /// whether the tab control is for a floating client
    float mFloatBorder;          /// border size for floating clients
    AreaTab* mActiveTab;         /// focused tab
    UIWindow mWindow;            /// tab control window
    std::vector<AreaTab*> mTabs; /// ordered tabs
};

} // namespace LD