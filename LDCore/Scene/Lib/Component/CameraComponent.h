#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI);
bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI);
bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
void unload_camera_component(SceneObj* scene, ComponentBase** camera);
void startup_camera_component(SceneObj* scene, ComponentBase** data);
void cleanup_camera_component(SceneObj* scene, ComponentBase** data);

} // namespace LD