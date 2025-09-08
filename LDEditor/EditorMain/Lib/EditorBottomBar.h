#pragma once

#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindow.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct EditorBottomBarInfo
{
    UIContext context;
    EditorTheme theme;
    Vec2 screenSize;
    float barHeight;
};

/// @brief Editor bottom bar UI.
class EditorBottomBar
{
public:
    /// @brief In-place startup.
    void startup(const EditorBottomBarInfo& info);

    /// @brief In-place cleanup.
    void cleanup();

    /// @brief Get window handle.
    UIWindow get_handle();

private:
    UIWindow mRoot;
    float mBottomBarHeight;
};

} // namespace LD