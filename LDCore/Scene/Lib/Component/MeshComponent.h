#pragma once

#include "../SceneObj.h"

namespace LD {

void init_mesh_component(ComponentBase** dstData);
bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID, String& err);
bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err);
bool unload_mesh_component(SceneObj* scene, ComponentBase** data, String& err);

} // namespace LD