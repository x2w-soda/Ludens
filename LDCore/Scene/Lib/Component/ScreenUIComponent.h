#pragma once

#include "../SceneObj.h"

namespace LD {

void init_screen_ui_component(ComponentBase** dstData);
bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID, std::string& err);
bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
bool unload_screen_ui_component(SceneObj* scene, ComponentBase** data, std::string& err);
bool startup_screen_ui_component(SceneObj* scene, ComponentBase** data, std::string& err);
bool cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data, std::string& err);

} // namespace LD