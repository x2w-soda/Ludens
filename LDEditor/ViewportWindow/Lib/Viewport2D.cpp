#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/Input.h>

#include "Viewport2D.h"

namespace LD {

Viewport2D::~Viewport2D()
{
    LD_ASSERT(!mCamera);
}

void Viewport2D::create(EditorContext ctx, const Vec2& sceneExtent)
{
    mCtx = ctx;
    mCamera = Camera2D::create(Camera2DInfo::extent(sceneExtent.x, sceneExtent.y));

    mCamera.set_position(Vec2(-200.0f, -200.0f));
}

void Viewport2D::destroy()
{
    if (mCamera)
    {
        Camera2D::destroy(mCamera);
        mCamera = {};
    }
}

void Viewport2D::imgui(const ViewportState& state)
{
    MouseButton btn;
    if (ui_top_mouse_down(btn))
    {
        if (btn == MOUSE_BUTTON_MIDDLE)
            mIsPanning = true;
    }

    mIsPanning = mIsPanning && Input::get_mouse(MOUSE_BUTTON_MIDDLE);

    bool dragBegin;
    Vec2 dragPos;
    if (ui_top_drag(btn, dragPos, dragBegin))
    {
        if (dragBegin)
            mDragPosPrevFrame = dragPos;

        mDragPosThisFrame = dragPos;
        Vec2 dragDelta = mDragPosThisFrame - mDragPosPrevFrame;
        mDragPosPrevFrame = mDragPosThisFrame;

        if (mIsPanning)
            mCamera.set_position(mCamera.get_position() - dragDelta);
    }
}

} // namespace LD