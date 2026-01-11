#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct InspectorWindowObj;

void eui_inspect_component(InspectorWindowObj& self, ComponentType type, void* comp);

} // namespace LD