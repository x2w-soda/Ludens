#pragma once

#include <Ludens/Scene/Scene.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct InspectorWindowObj;

void eui_inspect_component(InspectorWindowObj& self, Scene::Component comp);

} // namespace LD