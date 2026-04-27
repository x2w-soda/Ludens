#pragma once

#include "../SceneObj.h"

namespace LD {

extern struct PropertyMetaTable gSprite2DPropMetaTable;

void init_sprite_2d_component(ComponentBase** dstData);
bool load_sprite_2d_component_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D, std::string& err);
bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D, std::string& err);
bool clone_sprite_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err);
bool startup_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err);
bool cleanup_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err);
AssetID sprite_2d_component_get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex);
bool sprite_2d_component_set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID id);
AssetType sprite_2d_component_get_asset_type(SceneObj* scene, uint32_t assetSlotIndex);

} // namespace LD