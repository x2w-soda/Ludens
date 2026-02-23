#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, std::string& err);
bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_audio_source_component(SceneObj* scene, ComponentBase** source, std::string& err);
bool cleanup_audio_source_component(SceneObj* scene, ComponentBase** source, std::string& err);

} // namespace LD