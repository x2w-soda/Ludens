#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear);
bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
void unload_audio_source_component(SceneObj* scene, ComponentBase** source);
void cleanup_audio_source_component(SceneObj* scene, ComponentBase** source);

} // namespace LD