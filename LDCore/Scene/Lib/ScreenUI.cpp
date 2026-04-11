#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneDef.h>

#include "ScreenUI.h"

namespace LD {

struct ScreenUIObj
{
    UIContext ctx{};
    UILayer layer{};
    UIWorkspace space{};
};

ScreenUI ScreenUI::create(const ScreenUIInfo& info)
{
    ScreenUIObj* obj = heap_new<ScreenUIObj>(MEMORY_USAGE_SCENE);

    Rect screenRect(0.0f, 0.0f, info.extent.x, info.extent.y);

    UIContextInfo ctxI{};
    ctxI.font = info.font;
    ctxI.theme = info.theme;
    obj->ctx = UIContext::create(ctxI);
    obj->layer = obj->ctx.create_layer("screen");
    obj->space = obj->layer.create_workspace(screenRect);

    return ScreenUI(obj);
}

void ScreenUI::destroy(ScreenUI ui)
{
    ScreenUIObj* obj = ui.unwrap();

    UIContext::destroy(obj->ctx);

    heap_delete<ScreenUIObj>(obj);
}

void ScreenUI::update(const SceneUpdateTick& tick)
{
    LD_PROFILE_SCOPE;

    Rect rect = mObj->space.get_root_rect();
    if (rect.w != tick.extent.x || rect.h != tick.extent.y)
        mObj->space.set_rect(Rect(0.0f, 0.0f, tick.extent.x, tick.extent.y));

    mObj->ctx.update(tick.delta);
}

void ScreenUI::render(ScreenRenderComponent renderer)
{
    LD_PROFILE_SCOPE;

    mObj->layer.render(renderer);
}

void ScreenUI::input(const WindowEvent* event)
{
    LD_PROFILE_SCOPE;

    mObj->ctx.input_window_event(event);
}

UIWorkspace ScreenUI::workspace()
{
    return mObj->space;
}

} // namespace LD