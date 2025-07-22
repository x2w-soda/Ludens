#pragma once

#include "UIFWindow.h"
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/ScreenRender.h>
#include <cstdint>
#include <vector>

namespace LD {
namespace UIF {

using WindowAreaID = uint32_t;

struct WindowManagerInfo
{
    Vec2 screenSize;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
};

/// @brief A window manager to partition screen space into non-overlapping areas.
struct WindowManager : Handle<struct WindowManagerObj>
{
    typedef void (*SizeCallback)(Window window);
    typedef void (*RenderCallback)(Window window, ScreenRenderComponent renderer);

    static WindowManager create(const WindowManagerInfo& wmInfo);
    static void destroy(WindowManager wm);

    void update(float delta);

    /// @brief invokes RenderCallback on visible areas
    /// @param renderer screen space renderer dependency
    void render(ScreenRenderComponent renderer);

    WindowAreaID get_root_area();

    Window get_area_window(WindowAreaID area);

    /// @brief get visible windows in the workspace
    void get_workspace_windows(std::vector<Window>& windows);

    /// @brief split an area to make room for right
    /// @return new area from right partition
    WindowAreaID split_right(WindowAreaID areaID, float ratio);
};

} // namespace UIF
} // namespace LD