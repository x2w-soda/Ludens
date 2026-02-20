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
    mCamera = Camera2D::create(sceneExtent);
    mCamera.set_position(Vec2(0.0f, 0.0f));

    mCameraController = Camera2DController::create(mCamera);
}

void Viewport2D::destroy()
{
    if (mCameraController)
    {
        Camera2DController::destroy(mCameraController);
        mCameraController = {};
    }

    if (mCamera)
    {
        Camera2D::destroy(mCamera);
        mCamera = {};
    }
}

void Viewport2D::imgui(const ViewportState& state)
{
    Vec2 scroll;
    MouseButton btn;
    if (ui_top_mouse_down(btn))
    {
        if (btn == MOUSE_BUTTON_MIDDLE)
            mIsPanning = true;
    }
    if (ui_top_scroll(scroll))
    {
        mCameraController.accumulate_zoom_exp(scroll.y);
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
            mCamera.set_position(mCamera.get_position() - dragDelta / mCamera.get_zoom());
    }

    const Vec2* sceneMousePos = nullptr;
    if (state.sceneMousePos.x > 0.0f && state.sceneMousePos.y > 0.0f)
        sceneMousePos = &state.sceneMousePos;
    mCameraController.update(state.delta, sceneMousePos);
}

} // namespace LD