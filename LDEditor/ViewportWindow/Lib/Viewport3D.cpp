#include <Ludens/Header/MouseValue.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/Input.h>

#include "Viewport3D.h"

#define GIZMO_SCREEN_SIZE_Y 150.0f

namespace LD {

Viewport3D::~Viewport3D()
{
    LD_ASSERT(!mCamera);
}

void Viewport3D::create(EditorContext ctx)
{
    mCtx = ctx;

    CameraPerspectiveInfo cameraPI{};
    cameraPI.aspectRatio = 1.0f; // updated each frame
    cameraPI.fov = 45.0f * LD_PI / 180.0f;
    cameraPI.nearClip = 0.1f;
    cameraPI.farClip = 100.0f;
    mGizmo = Gizmo::create();
    mCamera = Camera::create(cameraPI, {0.0f, 0.0f, 0.0f});
    mCamera.set_pos(Vec3(-2.10f, 0.05f, 11.64f));
    mCameraController = CameraController::create(mCamera, 3.0f, 0.22f);
}

void Viewport3D::destroy()
{
    if (mCameraController)
    {
        CameraController::destroy(mCameraController);
        mCameraController = {};
    }

    if (mCamera)
    {
        Camera::destroy(mCamera);
        mCamera = {};
    }

    if (mGizmo)
    {
        Gizmo::destroy(mGizmo);
        mGizmo = {};
    }
}

void Viewport3D::imgui(ViewportState& state)
{
    Mat4 worldMat4;
    MouseValue mouseVal;
    Vec2 mousePos;
    bool begin;

    mCamera.set_aspect_ratio(state.sceneExtent.x / state.sceneExtent.y);
    ComponentView subject = mCtx.get_component_by_suid(state.gizmoSubjectSUID);

    // update gizmo center and scale
    if (subject && subject.get_world_mat4(worldMat4))
    {
        mGizmoCenter = (worldMat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f)).as_vec3();
        mGizmoScale = mCamera.screen_to_world_size(mGizmoCenter, state.sceneExtent.y, GIZMO_SCREEN_SIZE_Y);
    }

    // update camera controls
    if (mEnableCameraControls)
    {
        CameraController cc = mCameraController;

        if (Input::get_key(KEY_CODE_W))
            cc.move_forward();
        if (Input::get_key(KEY_CODE_S))
            cc.move_backward();

        if (Input::get_key(KEY_CODE_A))
            cc.move_left();
        if (Input::get_key(KEY_CODE_D))
            cc.move_right();

        if (Input::get_key(KEY_CODE_E))
            cc.move_world_up();
        if (Input::get_key(KEY_CODE_Q))
            cc.move_world_down();

        float dx, dy;
        if (Input::get_mouse_motion(dx, dy))
        {
            cc.view_pitch(-dy);
            cc.view_yaw(dx);
        }

        cc.update(state.delta);
    }

    // mouse picking in 3D viewport
    if (ui_top_mouse_down(mouseVal, mousePos))
    {
        if (mouseVal.button() == MOUSE_BUTTON_RIGHT)
            mEnableCameraControls = true;
        else if (mouseVal.button() == MOUSE_BUTTON_LEFT && state.sceneMousePos.x > 0.0f && state.sceneMousePos.y > 0.0f)
        {
            // update camera ray required for gizmo controls
            mGizmo.update(mCamera, state.sceneMousePos, state.sceneExtent);

            if (state.hoverGizmoID != 0)
                pick_hover_gizmo_id(state);
            else if (state.hoverRUID != 0)
                pick_hover_ruid(state);
            else // clear selection
            {
                state.hoverRUID = 0;
                pick_hover_ruid(state);
            }
        }
    }

    if (ui_top_mouse_up(mouseVal, mousePos))
    {
        if (mouseVal.button() == MOUSE_BUTTON_LEFT)
            mGizmo.end();
        else if (mouseVal.button() == MOUSE_BUTTON_RIGHT)
            mEnableCameraControls = false;
    }

    MouseButton mouseBtn;
    if (ui_top_drag(mouseBtn, mousePos, begin))
        drag(state, mouseBtn, mousePos, begin);
}

void Viewport3D::pick_hover_ruid(ViewportState& state)
{
    ComponentView comp = mCtx.get_component_by_ruid(state.hoverRUID);

    state.gizmoSubjectSUID = comp ? comp.suid() : (SUID)0;

    mCtx.set_selected_component(comp.cuid());
}

void Viewport3D::pick_hover_gizmo_id(ViewportState& state)
{
    GizmoPlane plane;
    GizmoAxis axis;
    bool ok;

    // writes back to subject transform during mouse drag window events
    // an object should be selected before gizmo mesh can even be selected
    ComponentView subject = mCtx.get_component_by_suid(state.gizmoSubjectSUID);
    LD_ASSERT(subject);

    // initialize subject world transform and gizmo center.
    Mat4 worldMat4;
    ok = subject.get_world_mat4(worldMat4);
    LD_ASSERT(ok);

    mGizmoCenter = (worldMat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f)).as_vec3();
    ok = decompose_mat4_to_transform(worldMat4, mSubjectWorldTransform);
    LD_ASSERT(ok);

    SceneOverlayGizmoID id = state.hoverGizmoID;

    switch (state.gizmoType)
    {
    case SCENE_OVERLAY_GIZMO_TRANSLATION:
        if (get_gizmo_axis(id, axis))
            mGizmo.begin_axis_translate(axis, mGizmoCenter);
        else if (get_gizmo_plane(id, plane))
            mGizmo.begin_plane_translate(plane, mGizmoCenter);
        break;
    case SCENE_OVERLAY_GIZMO_ROTATION:
        if (get_gizmo_plane(id, plane))
            mGizmo.begin_plane_rotate(plane, mGizmoCenter, get_plane_rotation(plane, mSubjectWorldTransform.rotationEuler));
        break;
    case SCENE_OVERLAY_GIZMO_SCALE:
        if (get_gizmo_axis(id, axis))
            mGizmo.begin_axis_scale(axis, mGizmoCenter, mSubjectWorldTransform.scale);
        break;
    default:
        break;
    }
}

void Viewport3D::drag(ViewportState& state, MouseButton btn, const Vec2& dragPos, bool begin)
{
    if (btn != MOUSE_BUTTON_LEFT)
        return;

    // update gizmo controls
    GizmoAxis axis;
    GizmoPlane plane;
    GizmoControl control = mGizmo.is_active(axis, plane);
    if (control == GIZMO_CONTROL_NONE)
        return;

    LD_ASSERT(state.gizmoSubjectSUID);
    TransformEx& worldT = mSubjectWorldTransform;
    const Vec2 scenePos(dragPos.x, dragPos.y - VIEWPORT_TOOLBAR_HEIGHT);

    // drag position is relative to window origin, i.e. already within sceneExtent range
    mGizmo.update(mCamera, scenePos, state.sceneExtent);

    switch (control)
    {
    case GIZMO_CONTROL_AXIS_TRANSLATION:
        worldT.position = mGizmo.get_axis_translate();
        break;
    case GIZMO_CONTROL_PLANE_TRANSLATION:
        worldT.position = mGizmo.get_plane_translate();
        break;
    case GIZMO_CONTROL_PLANE_ROTATION:
        switch (plane)
        {
        case GIZMO_PLANE_XY:
            worldT.rotationEuler.z = LD_TO_DEGREES(mGizmo.get_plane_rotate());
            break;
        case GIZMO_PLANE_XZ:
            worldT.rotationEuler.y = LD_TO_DEGREES(mGizmo.get_plane_rotate());
            break;
        case GIZMO_PLANE_YZ:
            worldT.rotationEuler.x = LD_TO_DEGREES(mGizmo.get_plane_rotate());
            break;
        }
        worldT.rotation = Quat::from_euler(worldT.rotationEuler);
        break;
    case GIZMO_CONTROL_AXIS_SCALE:
        worldT.scale = mGizmo.get_axis_scale();
        break;
    default:
        break;
    }

    // get inverse parent world matrix
    Mat4 parentInv(1.0f);
    ComponentView subject = mCtx.get_component_by_suid(state.gizmoSubjectSUID);
    ComponentView parent{};
    bool ok;

    if (subject && (parent = subject.get_parent()))
    {
        Mat4 parentWorldMat4(1.0f);
        ok = parent.get_world_mat4(parentWorldMat4);
        LD_ASSERT(ok);
        parentInv = Mat4::inverse(parentWorldMat4);
    }

    // get new local matrix from new world matrix and inverse parent world matrix
    Mat4 worldMat4 = worldT.as_mat4();
    Mat4 localMat4 = parentInv * worldMat4;

    // decompose local matrix to local transform
    TransformEx localTransform;
    ok = decompose_mat4_to_transform(localMat4, localTransform);
    LD_ASSERT(ok);

    subject.set_transform(localTransform);

    // update gizmo center to new world space position
    mGizmoCenter = (worldMat4 * Vec4(0.0f, 0.0f, 0.0f, 1.0f)).as_vec3();
}

} // namespace LD