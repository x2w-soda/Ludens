#include <Ludens/Scene/Component/Camera2DView.h>

#include "Camera2DComponent.h"

namespace LD {

static Rect clamp_rect(Rect rect)
{
    return Rect(std::clamp(rect.x, 0.0f, 1.0f), std::clamp(rect.y, 0.0f, 1.0f), std::clamp(rect.w, 0.0f, 1.0f), std::clamp(rect.h, 0.0f, 1.0f));
}

void init_camera_2d_component(ComponentBase** dstData)
{
    ComponentBase* base = *dstData;
    Camera2DComponent* dstCamera2D = (Camera2DComponent*)dstData;
    dstCamera2D->transform = base->transform2D;
    dstCamera2D->camera = {};
    dstCamera2D->viewport = Rect(0.0f, 0.0f, 1.0f, 1.0f);
    LD_ASSERT(dstCamera2D->transform);
}

bool clone_camera_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    Camera2DView srcCamera(srcData);
    LD_ASSERT(srcCamera);

    Rect viewport = srcCamera.get_viewport();
    Camera2DInfo info = srcCamera.get_info();
    return load_camera_2d_component(scene, (Camera2DComponent*)dstData, info, viewport, err);
}

bool load_camera_2d_component(SceneObj* scene, Camera2DComponent* camera, const Camera2DInfo& info, const Rect& viewport, std::string& err)
{
    if (camera->camera)
        Camera2D::destroy(camera->camera);

    camera->camera = Camera2D::create(info);
    camera->viewport = viewport;

    return (bool)camera->camera;
}

bool unload_camera_2d_component(SceneObj* scene, ComponentBase** cameraData, std::string& err)
{
    Camera2DComponent* camera = (Camera2DComponent*)cameraData;

    if (camera->camera)
    {
        Camera2D::destroy(camera->camera);
        camera->camera = {};
    }

    return true;
}

bool startup_camera_2d_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    Camera2DComponent* camera = (Camera2DComponent*)data;

    if (!camera->camera)
    {
        camera->camera = Camera2D::create(scene->extent);
    }

    return (bool)camera->camera;
}

bool cleanup_camera_2d_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    // Camera2D not destroyed until unload.
    return true;
}

Camera2DView::Camera2DView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_CAMERA_2D)
    {
        mData = comp.data();
        mCamera = (Camera2DComponent*)mData;
    }
}

Camera2DView::Camera2DView(Camera2DComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mCamera = comp;
    }
}

bool Camera2DView::load(const Camera2DInfo& info, const Rect& viewport, std::string& err)
{
    return load_camera_2d_component(sScene, mCamera, info, viewport, err);
}

Camera2DInfo Camera2DView::get_info()
{
    Camera2D cam = mCamera->camera;

    Camera2DInfo info{};
    info.position = cam.get_position();
    info.extent = cam.get_extent();
    info.zoom = cam.get_zoom();
    info.rotation = cam.get_rotation();

    return info;
}

Rect Camera2DView::get_viewport()
{
    return mCamera->viewport;
}

void Camera2DView::set_viewport(Rect viewport)
{
    mCamera->viewport = clamp_rect(viewport);
}

Vec2 Camera2DView::get_extent()
{
    return mCamera->camera.get_extent();
}

void Camera2DView::set_extent(Vec2 extent)
{
    mCamera->camera.set_extent(extent);
}

float Camera2DView::get_zoom()
{
    return mCamera->camera.get_zoom();
}

void Camera2DView::set_zoom(float zoom)
{
    if (zoom != 0.0f)
        mCamera->camera.set_zoom(zoom);
}

} // namespace LD