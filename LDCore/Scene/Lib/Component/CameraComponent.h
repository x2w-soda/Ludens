#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI, std::string& err);
bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI, std::string& err);
bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_camera_component(SceneObj* scene, ComponentBase** camera, std::string& err);
bool startup_camera_component(SceneObj* scene, ComponentBase** data, std::string& err);
bool cleanup_camera_component(SceneObj* scene, ComponentBase** data, std::string& err);

} // namespace LD