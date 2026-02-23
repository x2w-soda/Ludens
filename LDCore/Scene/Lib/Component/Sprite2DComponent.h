#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_sprite_2d_component_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D, std::string& err);
bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D, std::string& err);
bool clone_sprite_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err);

} // namespace LD