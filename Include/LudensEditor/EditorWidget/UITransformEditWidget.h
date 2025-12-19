#pragma once

#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct Transform;
struct Transform2D;

void eui_transform_edit(EditorTheme editorTheme, Transform* transform);
void eui_transform_2d_edit(EditorTheme editorTheme, Transform2D* transform2D);

} // namespace LD