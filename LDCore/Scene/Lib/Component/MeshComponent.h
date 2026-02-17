#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID);
bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
void unload_mesh_component(SceneObj* scene, ComponentBase** data);

} // namespace LD