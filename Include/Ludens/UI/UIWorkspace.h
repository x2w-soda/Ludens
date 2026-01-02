#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
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

    /// @brief Create and add a window to the workspace.
    UIWindow create_window(UIAreaID areaID, const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);

    /// @brief Destroy a window in the workspace.
    /// @note Deferred destruction until next context update.
    void destroy_window(UIWindow window);

    /// @brief Get all windows in this workspace.
    /// @param windows Outputs all windows inside the workspace, visible or not.
    void get_windows(Vector<UIWindow>& windows);

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