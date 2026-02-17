#include <Ludens/Profiler/Profiler.h>

#include "ScreenUI.h"

namespace LD {

struct ScreenUIObj
{
    UIContext ctx{};
    UIWorkspace space{};
};

ScreenUI ScreenUI::create(const ScreenUIInfo& info)
{
    ScreenUIObj* obj = heap_new<ScreenUIObj>(MEMORY_USAGE_SCENE);

    Rect screenRect(0.0f, 0.0f, info.extent.x, info.extent.y);
    UIContextInfo ctxI{};
    ctxI.fontAtlas = info.fontAtlas;
    ctxI.fontAtlasImage = info.fontAtlasImage;
    ctxI.theme = info.theme;
    obj->ctx = UIContext::create(ctxI);
    obj->space = obj->ctx.create_layer("screen").create_workspace(screenRect);

    return ScreenUI(obj);
}

void ScreenUI::destroy(ScreenUI ui)
{
    ScreenUIObj* obj = ui.unwrap();

    UIContext::destroy(obj->ctx);

    heap_delete<ScreenUIObj>(obj);
}

void ScreenUI::update(float delta)
{
    LD_PROFILE_SCOPE;

    mObj->ctx.update(delta);
}

void ScreenUI::resize(const Vec2& extent)
{
    LD_PROFILE_SCOPE;

    mObj->space.set_rect(Rect(0.0f, 0.0f, extent.x, extent.y));
}

UIWorkspace ScreenUI::workspace()
{
    return mObj->space;
}

} // namespace LD