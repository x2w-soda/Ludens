#pragma once

#include <Ludens/Scene/Scene.h>

#include <LudensEditor/EditorWidget/EUIProp.h>

namespace LD {

struct EditorContext;
struct EUIComponentStorage;

void eui_inspect_component(EditorContext& ctx, ComponentView comp);
void eui_inspect_component_script(EditorContext& ctx, ComponentView comp);

} // namespace LD