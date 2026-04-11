#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIContext.h>

namespace LD {

struct SceneUpdateTick;

struct ScreenUIInfo
{
    Vec2 extent;
    UIFont font;
    UITheme theme;
};

/// @brief Scene level screen UI context.
struct ScreenUI : Handle<struct ScreenUIObj>
{
    static ScreenUI create(const ScreenUIInfo& info);
    static void destroy(ScreenUI ui);

    void update(const SceneUpdateTick& tick);
    void render(ScreenRenderComponent renderer);
    void input(const WindowEvent* event);

    UIWorkspace workspace();
};

} // namespace LD