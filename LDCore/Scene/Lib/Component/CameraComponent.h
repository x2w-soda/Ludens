#pragma once

#include "../SceneObj.h"

namespace LD {

void init_camera_component(ComponentBase** dstData);
bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI, String& err);
bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI, String& err);
bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err);
bool unload_camera_component(SceneObj* scene, ComponentBase** camera, String& err);
bool startup_camera_component(SceneObj* scene, ComponentBase** data, String& err);
bool cleanup_camera_component(SceneObj* scene, ComponentBase** data, String& err);

} // namespace LD