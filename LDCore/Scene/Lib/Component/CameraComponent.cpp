#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/CameraView.h>

#include "CameraComponent.h"

namespace LD {

static const CameraComponent sDefaultCamera = {
    .transform = {},
    .camera = {},
    .isMainCamera = false,
};

void init_camera_component(ComponentBase** dstData)
{
    memcpy(dstData, &sDefaultCamera, sizeof(CameraComponent));
}

bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI, std::string& err)
{
    LD_PROFILE_SCOPE;

    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    return true;
}

bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI, std::string& err)
{
    LD_PROFILE_SCOPE;

    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    return true;
}

bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    CameraView srcCamera(srcData);
    LD_ASSERT(srcCamera);

    CameraPerspectiveInfo perspectiveI;
    CameraOrthographicInfo orthoI;
    if (srcCamera.is_perspective())
    {
        if (!srcCamera.get_perspective_info(perspectiveI) || !load_camera_component_perspective(scene, (CameraComponent*)dstData, perspectiveI, err))
            return false;
    }
    else
    {
        if (!srcCamera.get_orthographic_info(orthoI) || !load_camera_component_orthographic(scene, (CameraComponent*)dstData, orthoI, err))
            return false;
    }

    if (srcCamera.is_main_camera())
        ((CameraComponent*)dstData)->isMainCamera = true;

    return true;
}

bool unload_camera_component(SceneObj* scene, ComponentBase** cameraData, std::string& err)
{
    CameraComponent* camera = (CameraComponent*)cameraData;

    if (camera->camera)
    {
        LD::Camera::destroy(camera->camera);
        camera->camera = {};
    }

    return true;
}

bool startup_camera_component(SceneObj* scene, ComponentBase** cameraData, std::string& err)
{
    ComponentBase* base = *cameraData;
    auto* camera = (CameraComponent*)cameraData;

    /*
    if (scene->mainCameraC)
        return true; // not an error

    scene->mainCameraC = camera;
    */

    return true;
}

bool cleanup_camera_component(SceneObj* scene, ComponentBase** cameraData, std::string& err)
{
    /*
    if (scene->mainCameraC == (CameraComponent*)cameraData)
        scene->mainCameraC = nullptr;
    */

    return true;
}

CameraView::CameraView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_CAMERA)
    {
        mData = comp.data();
        mCamera = (CameraComponent*)mData;
    }
}

CameraView::CameraView(CameraComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mCamera = comp;
    }
}

bool CameraView::load_perspective(const CameraPerspectiveInfo& info)
{
    std::string err;

    return load_camera_component_perspective(sScene, mCamera, info, err);
}

bool CameraView::load_orthographic(const CameraOrthographicInfo& info)
{
    std::string err;

    return load_camera_component_orthographic(sScene, mCamera, info, err);
}

bool CameraView::is_main_camera()
{
    return mCamera->isMainCamera;
}

bool CameraView::is_perspective()
{
    LD_ASSERT(mCamera->camera);

    return mCamera->camera.is_perspective();
}

bool CameraView::get_perspective_info(CameraPerspectiveInfo& outInfo)
{
    LD_ASSERT(mCamera->camera);

    if (mCamera->camera.is_perspective())
    {
        outInfo = mCamera->camera.get_perspective();
        return true;
    }

    return false;
}

bool CameraView::get_orthographic_info(CameraOrthographicInfo& outInfo)
{
    LD_ASSERT(mCamera->camera);

    if (!mCamera->camera.is_perspective())
    {
        outInfo = mCamera->camera.get_orthographic();
        return true;
    }

    return false;
}

void CameraView::set_perspective(const CameraPerspectiveInfo& info)
{
    LD_ASSERT(mCamera->camera);

    mCamera->camera.set_perspective(info);
}

void CameraView::set_orthographic(const CameraOrthographicInfo& info)
{
    LD_ASSERT(mCamera->camera);

    mCamera->camera.set_orthographic(info);
}

} // namespace LD