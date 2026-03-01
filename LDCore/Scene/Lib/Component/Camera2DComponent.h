#pragma once

#include "../SceneObj.h"

namespace LD {

void init_camera_2d_component(ComponentBase** dstData);
bool clone_camera_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool load_camera_2d_component(SceneObj* scene, Camera2DComponent* camera, const Camera2DInfo& info, const Rect& viewport, std::string& err);
bool unload_camera_2d_component(SceneObj* scene, ComponentBase** camera, std::string& err);
bool startup_camera_2d_component(SceneObj* scene, ComponentBase** data, std::string& err);
bool cleanup_camera_2d_component(SceneObj* scene, ComponentBase** data, std::string& err);

} // namespace LD