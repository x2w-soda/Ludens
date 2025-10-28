#include "EditorBottomBar.h"

namespace LD {

void EditorBottomBar::startup(const EditorBottomBarInfo& info)
{
    UIContext ctx = info.context;
    mBottomBarHeight = info.barHeight;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.childGap = 6.0f;
    layoutI.sizeX = UISize::fixed(info.screenSize.x);
    layoutI.sizeY = UISize::fixed(mBottomBarHeight);
    UIWindowInfo windowI{};
    windowI.name = "EditorBottomBar";
    windowI.layer = info.layer;
    windowI.defaultMouseControls = false;

    mRoot = ctx.add_window(layoutI, windowI, nullptr);
    mRoot.set_pos(Vec2(0.0f, info.screenSize.y - mBottomBarHeight));
    mRoot.set_user(this);
}

void EditorBottomBar::cleanup()
{
}

UIWindow EditorBottomBar::get_handle()
{
    return mRoot;
}

} // namespace LD