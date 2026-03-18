#include <Ludens/Gizmo/Gizmo2D.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/Input.h>

#include "Viewport2D.h"

#define DRAG_BEGIN_THRESHOLD 5.0f

namespace LD {

Viewport2D::~Viewport2D()
{
    LD_ASSERT(!mCamera);
}

void Viewport2D::create(EditorContext ctx)
{
    mCtx = ctx;
    mCamera = Camera2D::create(Vec2(0.0f));
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
    mCamera.set_extent(state.viewportExtent);

    mMouseScenePos.reset();
    mMouseWorldPos.reset();
    const Vec2* pSceneMousePos = nullptr;

    if (state.sceneMousePos.x > 0.0f && state.sceneMousePos.y > 0.0f)
    {
        pSceneMousePos = &state.sceneMousePos;
        mMouseScenePos = state.sceneMousePos;
        mMouseWorldPos = mCamera.get_world_position(mMouseScenePos.value());
    }

    mCameraController.update(state.delta, pSceneMousePos);

    Vec2 scroll;
    Vec2 mousePos;
    MouseValue mouseVal;
    MouseButton mouseBtn;
    if (ui_top_mouse_down(mouseVal, mousePos))
        mouse_button_down(state, mouseVal);
    if (ui_top_mouse_up(mouseVal, mousePos))
        mouse_button_up(state, mouseVal);
    if (ui_top_scroll(scroll))
    {
        mCameraController.accumulate_zoom_exp(scroll.y);
    }

    mIsPanning = mIsPanning && Input::get_mouse(MOUSE_BUTTON_MIDDLE);

    bool dragBegin;
    Vec2 dragPos;
    if (ui_top_drag(mouseBtn, dragPos, dragBegin))
    {
        if (dragBegin)
            drag_begin(dragPos);

        mDragPosThisFrame = dragPos;
        Vec2 dragScreenDelta = mDragPosThisFrame - mDragPosPrevFrame;
        Vec2 dragWorldDelta = dragScreenDelta / mCamera.get_zoom();
        mDragPosPrevFrame = mDragPosThisFrame;

        if (!mIsDragging && (mDragPosThisFrame - mDragBeginMousePos).length() > DRAG_BEGIN_THRESHOLD)
            mIsDragging = true;

        if (mIsPanning)
            mCamera.set_position(mCamera.get_position() - dragWorldDelta);
        else
            drag_controls(state);
    }
}

void Viewport2D::drag_begin(Vec2 dragBeginPos)
{
    mDragBeginMousePos = dragBeginPos;
    mDragPosPrevFrame = dragBeginPos;
    mDragBeginMouseOffset = {};
    mDragMouseOffsetDeg = 0.0f;
    mDragBeginCompWorldDeg = 0.0f;
    mDragBeginCompWorldScale = Vec2(1.0f, 1.0f);

    Transform2D worldTransform;
    ComponentView selectedComp = mCtx.get_selected_component_view();
    if (selectedComp && selectedComp.get_world_transform_2d(worldTransform))
    {
        mDragBeginCompWorldDeg = worldTransform.rotation;
        mDragBeginCompWorldScale = worldTransform.scale;

        if (mMouseWorldPos)
        {
            const Vec2 mouseWorldPos = mMouseWorldPos.value();
            mDragBeginMouseOffset = worldTransform.position - mouseWorldPos;
            mDragMouseOffsetDeg = LD_TO_DEGREES(LD_ATAN2(mouseWorldPos.y - worldTransform.position.y, mouseWorldPos.x - worldTransform.position.x));
        }
    }
}

void Viewport2D::drag_controls(const ViewportState& state)
{
    Transform2D childLocal;
    ComponentView selectedComp = mCtx.get_selected_component_view();

    if (!mIsDragging || !mMouseWorldPos || !selectedComp || !selectedComp.get_transform_2d(childLocal))
        return;

    const float dragBeginMouseDist = mDragBeginMouseOffset.length();
    const Vec2 mouseWorldPos = mMouseWorldPos.value();
    Transform2D parentWorld = Transform2D::identity();
    ComponentView parentComp = selectedComp.get_parent();
    if (parentComp)
        (void)parentComp.get_world_transform_2d(parentWorld);

    // Solve for child local Transform given mouse cursor world position
    Transform2D newChildLocal = Transform2D::identity();

    switch (state.gizmoType)
    {
    case SCENE_OVERLAY_GIZMO_TRANSLATION:
        newChildLocal = Gizmo2D::translate(childLocal, parentWorld, mouseWorldPos, mDragBeginMouseOffset);
        selectedComp.set_transform_2d(newChildLocal);
        break;
    case SCENE_OVERLAY_GIZMO_ROTATION:
        newChildLocal = Gizmo2D::rotate(childLocal, parentWorld, mouseWorldPos, mDragMouseOffsetDeg, mDragBeginCompWorldDeg);
        selectedComp.set_transform_2d(newChildLocal);
        break;
    case SCENE_OVERLAY_GIZMO_SCALE:
        newChildLocal = Gizmo2D::scale(childLocal, parentWorld, mouseWorldPos, dragBeginMouseDist, mDragBeginCompWorldScale);
        selectedComp.set_transform_2d(newChildLocal);
        break;
    default:
        break;
    }
}

void Viewport2D::mouse_button_down(const ViewportState& state, MouseValue mouseVal)
{
    if (mouseVal.button() == MOUSE_BUTTON_MIDDLE)
        mIsPanning = true;
    else if (mouseVal.button() == MOUSE_BUTTON_LEFT && mMouseWorldPos)
    {
        const Vec2 mouseWorldPos = mMouseWorldPos.value();
        ComponentView comp = mCtx.get_scene().get_2d_component_by_position(mouseWorldPos);
        mCtx.set_selected_component(comp ? comp.cuid() : CUID(0));
    }
}

void Viewport2D::mouse_button_up(const ViewportState& state, MouseValue mouseVal)
{
    mIsDragging = false;
}

} // namespace LD