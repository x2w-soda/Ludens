#pragma once

#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct Transform;

void eui_transform_edit(EditorTheme editorTheme, Transform* transform);

} // namespace LD