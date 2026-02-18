#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/UI/UIWindow.h>

namespace LD {

struct ScreenRenderComponent;

/// @brief Window area identifier distributed by workspace, zero is invalid ID.
using UIAreaID = uint32_t;

/// @brief A container for UI windows.
struct UIWorkspace : Handle<struct UIWorkspaceObj>
{
    /// @brief Render all windows in this workspace.
    void render(ScreenRenderComponent& renderer);

    /// @brief Raise workspace to top in layer.
    void raise();

    /// @brief If hidden, skips rendering for all UIWindows in this workspace.
    /// @note Each UIWindow has their own subtree visbility mask via UIWidget::set_visible.
    void set_visible(bool isVisible);

    /// @brief Set workspace rect, triggers resize callbacks for docked windows.
    void set_rect(const Rect& rect);

    /// @brief Set workspace position, does not resize docked windows.
    void set_pos(const Vec2& pos);

    /// @brief Create and add a window to the workspace, the window is docked in the desginated area.
    UIWindow create_window(UIAreaID areaID, const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);

    /// @brief Create and add a window to the workspace, the window is not docked.
    UIWindow create_float_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);

    /// @brief Destroy a window in the workspace.
    /// @note Deferred destruction until next context update.
    void destroy_window(UIWindow window);

    /// @brief Get all windows in this workspace.
    /// @param windows Outputs all windows inside the workspace, visible or not.
    void get_docked_windows(Vector<UIWindow>& windows);

    /// @brief Get hash that uniquely identifies workspace throughout its UI context.
    Hash64 get_hash();

    /// @brief Get root area ID.
    UIAreaID get_root_id();

    /// @brief Get root area rect.
    Rect get_root_rect();

    /// @brief Get the docked window from ID.
    UIWindow get_area_window(UIAreaID areaID);

    /// @brief Split an area to make room for right.
    /// @return New area from right partition.
    UIAreaID split_right(UIAreaID areaID, float ratio);

    /// @brief Split an area to make room for bottom.
    /// @return New area from bottom partition.
    UIAreaID split_bottom(UIAreaID areaID, float ratio);
};

} // namespace LD