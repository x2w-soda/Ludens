#include "Camera2DComponent.h"

namespace LD {

static const Camera2DComponent sDefaultCamera = {
    .base = nullptr,
    .transform = {},
    .camera = {},
    .viewport = {0.0f, 0.0f, 1.0f, 1.0f},
};

void init_camera_2d_component(ComponentBase** dstData)
{
    memcpy(dstData, &sDefaultCamera, sizeof(Camera2DComponent));
}

bool clone_camera_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    Camera2DView srcCamera(srcData);
    LD_ASSERT(srcCamera);

    ((Camera2DComponent*)dstData)->transform = ((Camera2DComponent*)srcData)->transform;

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
        return false;

    return true;
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

} // namespace LD