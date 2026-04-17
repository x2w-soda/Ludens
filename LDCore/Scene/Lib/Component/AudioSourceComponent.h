#pragma once

#include "../SceneObj.h"

namespace LD {

void init_audio_source_component(ComponentBase** dstData);
bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, std::string& err);
bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_audio_source_component(SceneObj* scene, ComponentBase** source, std::string& err);
bool cleanup_audio_source_component(SceneObj* scene, ComponentBase** source, std::string& err);
AssetID audio_source_component_get_asset(SceneObj* scene, ComponentBase** source, uint32_t assetSlotIndex);
bool audio_source_component_set_asset(SceneObj* scene, ComponentBase** source, uint32_t assetSlotIndex, AssetID id);

} // namespace LD