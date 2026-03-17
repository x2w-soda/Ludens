#include <Ludens/Header/MouseValue.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/Input.h>

#include "Viewport2D.h"

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
    {
        mouseBtn = mouseVal.button();
        if (mouseBtn == MOUSE_BUTTON_MIDDLE)
            mIsPanning = true;
        else if (mouseBtn == MOUSE_BUTTON_LEFT && mMouseWorldPos)
        {
            ComponentView comp = mCtx.get_scene().get_2d_component_by_position(mMouseWorldPos.value());
            mCtx.set_selected_component(comp ? comp.cuid() : (CUID)0);
        }
    }
    if (ui_top_mouse_up(mouseVal, mousePos))
    {
        mIsDragging = false;
    }
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
        {
            mIsDragging = true;
            mDragPosPrevFrame = dragPos;
            mDragMouseDelta = {};

            Transform2D worldTransform;
            ComponentView selectedComp = mCtx.get_selected_component_view();
            if (selectedComp && selectedComp.get_world_transform_2d(worldTransform) && mMouseWorldPos)
                mDragMouseDelta = worldTransform.position - mMouseWorldPos.value();
        }

        mDragPosThisFrame = dragPos;
        Vec2 dragScreenDelta = mDragPosThisFrame - mDragPosPrevFrame;
        Vec2 dragWorldDelta = dragScreenDelta / mCamera.get_zoom();
        mDragPosPrevFrame = mDragPosThisFrame;

        if (mIsPanning)
            mCamera.set_position(mCamera.get_position() - dragWorldDelta);
        else
            drag_controls(state);
    }
}

void Viewport2D::drag_controls(const ViewportState& state)
{
    Transform2D childLocal;
    ComponentView selectedComp = mCtx.get_selected_component_view();

    if (!mIsDragging || !mMouseWorldPos || !selectedComp || !selectedComp.get_transform_2d(childLocal))
        return;

    Transform2D parentWorld = Transform2D::identity();
    ComponentView parentComp = selectedComp.get_parent();

    // Solve for child local Transform given mouse cursor world position
    switch (state.gizmoType)
    {
    case SCENE_OVERLAY_GIZMO_TRANSLATION:
    {
        const Vec2 targetWorldPos = mMouseWorldPos.value() + mDragMouseDelta;

        if (!parentComp || !parentComp.get_world_transform_2d(parentWorld))
        {
            childLocal.position = targetWorldPos;
            selectedComp.set_transform_2d(childLocal);
            break;
        }

        Vec2 delta = targetWorldPos - parentWorld.position;
        Vec4 newLocalPos = Mat4::rotate(LD_TO_RADIANS(-parentWorld.rotation), Vec3(0.0f, 0.0f, 1.0f)) * Vec4(delta, 0.0f, 1.0f);

        if (!is_zero_epsilon(parentWorld.scale.x))
            newLocalPos.x /= parentWorld.scale.x;
        if (!is_zero_epsilon(parentWorld.scale.y))
            newLocalPos.y /= parentWorld.scale.y;

        childLocal.position = Vec2(newLocalPos.x, newLocalPos.y);

        selectedComp.set_transform_2d(childLocal);
        break;
    }
    default:
        break;
    }
}

} // namespace LD