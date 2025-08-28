#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIContext.h>
#include <cstdint>
#include <vector>

namespace LD {

/// @brief Window area identifier distributed by the window manager, zero is invalid ID.
using UIWindowAreaID = uint32_t;

struct UIWindowManagerInfo
{
    Vec2 screenSize;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    UITheme theme;
};

/// @brief A window manager to partition screen space into non-overlapping areas.
///        Contains its own UIContext and manages windows and widgets.
struct UIWindowManager : Handle<struct UIWindowManagerObj>
{
    /// @brief Create window manager, which will also create a UI context.
    static UIWindowManager create(const UIWindowManagerInfo& wmInfo);

    /// @brief Destroy window manager and its UI context.
    static void destroy(UIWindowManager wm);

    /// @brief Drive the internal UI context with delta time.
    void update(float delta);

    /// @brief Update screen size, recalculates area and invokes window resize callback.
    void resize(const Vec2& screenSize);

    /// @brief Invokes RenderCallback on visible areas.
    /// @param renderer Screen space renderer dependency.
    void render(ScreenRenderComponent renderer);

    /// @brief Set window title to be displayed in tab.
    void set_window_title(UIWindowAreaID areaID, const char* title);

    /// @brief Set callback to be invoked during UIWindowManager::resize.
    void set_on_window_resize(UIWindowAreaID areaID, void (*onWindowResize)(UIWindow window, const Vec2& size));

    /// @brief Get the underlying UI context
    UIContext get_context();

    /// @brief Get root area ID.
    UIWindowAreaID get_root_area();

    /// @brief Get the top bar window handle.
    UIWindow get_topbar_window();

    /// @brief Get the active tab window in an area. 
    UIWindow get_area_window(UIWindowAreaID areaID);

    /// @brief get visible windows in the workspace
    void get_workspace_windows(std::vector<UIWindow>& windows);

    /// @brief split an area to make room for right
    /// @return new area from right partition
    UIWindowAreaID split_right(UIWindowAreaID areaID, float ratio);

    /// @brief Split an area to make room for bottom.
    /// @return New area from bottom partition.
    UIWindowAreaID split_bottom(UIWindowAreaID areaID, float ratio);

    /// @brief Create floating window area.
    /// @param rect Floating window area rect.
    /// @return New area for floating window.
    UIWindowAreaID create_float(const Rect& rect);
};

} // namespace LD