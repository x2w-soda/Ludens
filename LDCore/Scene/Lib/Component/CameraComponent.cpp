#include <Ludens/Profiler/Profiler.h>

#include "CameraComponent.h"

namespace LD {

bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI)
{
    LD_PROFILE_SCOPE;

    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    camera->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI)
{
    LD_PROFILE_SCOPE;

    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    camera->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::Camera srcCamera(srcData);
    LD_ASSERT(srcCamera);

    CameraPerspectiveInfo perspectiveI;
    CameraOrthographicInfo orthoI;
    if (srcCamera.is_perspective())
    {
        if (!srcCamera.get_perspective_info(perspectiveI) || !load_camera_component_perspective(scene, (CameraComponent*)dstData, perspectiveI))
            return false;
    }
    else
    {
        if (!srcCamera.get_orthographic_info(orthoI) || !load_camera_component_orthographic(scene, (CameraComponent*)dstData, orthoI))
            return false;
    }

    if (srcCamera.is_main_camera())
        ((CameraComponent*)dstData)->isMainCamera = true;

    return true;
}

void unload_camera_component(SceneObj* scene, ComponentBase** cameraData)
{
    CameraComponent* camera = (CameraComponent*)cameraData;

    if (camera->camera)
    {
        LD::Camera::destroy(camera->camera);
        camera->camera = {};
    }

    camera->base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

void startup_camera_component(SceneObj* scene, ComponentBase** cameraData)
{
    ComponentBase* base = *cameraData;
    auto* camera = (CameraComponent*)cameraData;

    if (scene->mainCameraC)
        return;

    scene->mainCameraC = camera;

    const Vec3 mainCameraTarget(0.0f, 0.0f, 1.0f);
}

void cleanup_camera_component(SceneObj* scene, ComponentBase** cameraData)
{
    if (scene->mainCameraC == (CameraComponent*)cameraData)
        scene->mainCameraC = nullptr;
}

Scene::Camera::Camera(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_CAMERA)
    {
        mData = comp.data();
        mCamera = (CameraComponent*)mData;
    }
}

Scene::Camera::Camera(CameraComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mCamera = comp;
    }
}

bool Scene::Camera::load_perspective(const CameraPerspectiveInfo& info)
{
    return load_camera_component_perspective(sScene, mCamera, info);
}

bool Scene::Camera::load_orthographic(const CameraOrthographicInfo& info)
{
    return load_camera_component_orthographic(sScene, mCamera, info);
}

bool Scene::Camera::is_main_camera()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mCamera->isMainCamera;
}

bool Scene::Camera::is_perspective()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mCamera->camera.is_perspective();
}

bool Scene::Camera::get_perspective_info(CameraPerspectiveInfo& outInfo)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    if (mCamera->camera.is_perspective())
    {
        outInfo = mCamera->camera.get_perspective();
        return true;
    }

    return false;
}

bool Scene::Camera::get_orthographic_info(CameraOrthographicInfo& outInfo)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    if (!mCamera->camera.is_perspective())
    {
        outInfo = mCamera->camera.get_orthographic();
        return true;
    }

    return false;
}

void Scene::Camera::set_perspective(const CameraPerspectiveInfo& info)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mCamera->camera.set_perspective(info);
}

void Scene::Camera::set_orthographic(const CameraOrthographicInfo& info)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mCamera->camera.set_orthographic(info);
}

} // namespace LD