#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIContext.h>

namespace LD {

/// @brief User of a UIWidget subtree, provides bindings and drives the subtree.
class UIDriver
{
public:
    UIDriver(const View& lua);
    ~UIDriver();

    bool attach();
    bool detach();
    bool update(float delta);

private:
};

struct ScreenUIInfo
{
    Vec2 extent;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    UITheme theme;
};

/// @brief Scene level screen UI context.
struct ScreenUI : Handle<struct ScreenUIObj>
{
    static ScreenUI create(const ScreenUIInfo& info);
    static void destroy(ScreenUI ui);

    void update(float delta);
    void resize(const Vec2& extent);

    UIWorkspace workspace();
};

} // namespace LD