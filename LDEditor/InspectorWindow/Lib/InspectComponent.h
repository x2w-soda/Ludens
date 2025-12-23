#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct EInspectorWindowObj;

void eui_inspect_component(EInspectorWindowObj& self, ComponentType type, void* comp);

} // namespace LD