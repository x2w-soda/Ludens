#pragma once

#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIWindowManager.h>

namespace LD {

/// @brief Tabs allow multiple clients to occupy the same area.
///        Only a single tab is active in an area at a time.
struct AreaTab
{
    UIPanelWidget rootW;                     /// tab panel widget
    UITextWidget titleTextW;                 /// tab title text widget
    UITextWidget closeW;                     /// close button widget
    UIWindow client;                         /// client window of this tab
    UIWMClientResizeCallback onClientResize; /// client window resize callback
    UIWMClientCloseCallback onClientClose;   /// client window close callback
    void* user;                              /// client window user
    bool shouldClose;                        /// hint for the AreaTabConrol to close the tab

    AreaTab(UIWindow client, UIWindow tabControl, void* user);

    static void on_close(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
};

/// @brief class for the window manager to control tabs in an area
class AreaTabControl
{
public:
    void startup_as_leaf(UIContext ctx, const Rect& area);
    void startup_as_float(UIContext ctx, const Rect& area, float border);
    void cleanup();

    void add_tab(UIWindow client, void* user);

    AreaTab* get_active_tab();

    inline size_t get_tab_count() const { return mTabs.size(); }

    void show();
    void hide();
    void draw(ScreenRenderComponent renderer);

    /// @brief Invalidate the area rect.
    /// @param area The new area containing tabs and clients
    void invalidate_area(const Rect& area);

private:
    void invalidate_float_area(const Rect& area);
    void invalidate_leaf_area(const Rect& area);
    void invalidate_client_pos(const Vec2& pos);
    void invalidate_client_size(const Vec2& size);
    void invalidate_client_area(const Rect& area);

    Rect get_leaf_client_area(const Rect& nodeArea);
    Rect get_float_client_area(const Rect& nodeArea);

    static void on_update(UIWidget widget, float delta);
    static void on_float_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    static void on_float_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_float_hover(UIWidget widget, UIEvent event);

private:
    bool mIsFloat;               /// whether the tab control is for a floating client
    float mFloatBorder;          /// border size for floating clients
    AreaTab* mActiveTab;         /// focused tab
    UIWindow mWindow;            /// tab control window
    UIContext mCtx;              /// UI context handle
    std::vector<AreaTab*> mTabs; /// ordered tabs
    Vec2 mDragOffset;
    Vec2 mDragBeginPos;
    Vec2 mDragBeginSize;
    bool mDragMove;
    bool mDragResize;
};

} // namespace LD