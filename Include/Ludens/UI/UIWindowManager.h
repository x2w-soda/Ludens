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
using UIWMAreaID = uint32_t;

typedef void (*UIWMClientResizeCallback)(UIWindow client, const Vec2& size, void* user);
typedef void (*UIWMClientCloseCallback)(UIWindow client, void* user);

struct UIWMClientInfo
{
    UIWindow client;                         /// user provides UI window as client
    UIWMClientResizeCallback resizeCallback; /// invoked when WM resizes the client
    void* user;                              /// dependency injection during client callbacks
};

struct UIWindowManagerInfo
{
    Vec2 screenSize;
    float topBarHeight;
    float bottomBarHeight;
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
    void set_window_title(UIWMAreaID areaID, const char* title);

    /// @brief Set callback to be invoked during UIWindowManager::resize.
    void set_resize_callback(UIWMAreaID areaID, UIWMClientResizeCallback callback);

    /// @brief Set callback to be invoked when a Client area is closed.
    void set_close_callback(UIWMAreaID areaID, UIWMClientCloseCallback callback);

    /// @brief Get the underlying UI context
    UIContext get_context();

    /// @brief Get root area ID.
    UIWMAreaID get_root_area();

    /// @brief Get the active tab window in an area.
    UIWindow get_area_window(UIWMAreaID areaID);

    /// @brief get visible windows in the workspace
    void get_workspace_windows(std::vector<UIWindow>& windows);

    /// @brief split an area to make room for right
    /// @return new area from right partition
    UIWMAreaID split_right(UIWMAreaID areaID, float ratio);

    /// @brief Split an area to make room for bottom.
    /// @return New area from bottom partition.
    UIWMAreaID split_bottom(UIWMAreaID areaID, float ratio);

    /// @brief Create floating window area.
    /// @return New area for floating window.
    UIWMAreaID create_float(const UIWMClientInfo& clientI);

    /// @brief Set the position of a floating area such that it is centered in screen.
    void set_float_pos_centered(UIWMAreaID areaID);

    /// @brief Set the position of a floating area.
    void set_float_pos(UIWMAreaID areaID, const Vec2& pos);

    /// @brief Make a floating area visible.
    void show_float(UIWMAreaID areaID);

    /// @brief Make a floating area invisible.
    void hide_float(UIWMAreaID areaID);
};

} // namespace LD