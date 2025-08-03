#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIContext.h>
#include <cstdint>
#include <vector>

namespace LD {

using UIWindowAreaID = uint32_t;

struct UIWindowManagerInfo
{
    Vec2 screenSize;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
};

/// @brief A window manager to partition screen space into non-overlapping areas.
struct UIWindowManager : Handle<struct UIWindowManagerObj>
{
    static UIWindowManager create(const UIWindowManagerInfo& wmInfo);
    static void destroy(UIWindowManager wm);

    void update(float delta);

    /// @brief update screen size, recalculates area and invokes window resize callback
    void resize(const Vec2& screenSize);

    /// @brief invokes RenderCallback on visible areas
    /// @param renderer screen space renderer dependency
    void render(ScreenRenderComponent renderer);

    /// @brief set callback to be invoked during UIWindowManager::resize
    void set_on_window_resize(UIWindowAreaID areaID, void (*onWindowResize)(UIWindow window, const Vec2& size));

    /// @brief get the underlying UI context
    UIContext get_context();

    UIWindowAreaID get_root_area();

    UIWindow get_topbar_window();

    UIWindow get_area_window(UIWindowAreaID areaID);

    /// @brief get visible windows in the workspace
    void get_workspace_windows(std::vector<UIWindow>& windows);

    /// @brief split an area to make room for right
    /// @return new area from right partition
    UIWindowAreaID split_right(UIWindowAreaID areaID, float ratio);

    /// @brief Split an area to make room for bottom.
    /// @return New area from bottom partition.
    UIWindowAreaID split_bottom(UIWindowAreaID areaID, float ratio);
};

} // namespace LD