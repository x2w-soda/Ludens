#pragma once

#include "../SceneObj.h"

namespace LD {

bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID);
bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
void unload_screen_ui_component(SceneObj* scene, ComponentBase** data);
void startup_screen_ui_component(SceneObj* scene, ComponentBase** data);
void cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data);

} // namespace LD