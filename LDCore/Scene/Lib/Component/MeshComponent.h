#pragma once

#include "../SceneObj.h"

namespace LD {

void init_mesh_component(ComponentBase** dstData);
bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID, std::string& err);
bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_mesh_component(SceneObj* scene, ComponentBase** data, std::string& err);

} // namespace LD