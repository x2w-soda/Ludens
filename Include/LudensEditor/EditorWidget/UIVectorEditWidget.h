#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

void eui_u32_edit(EditorTheme theme, const char* label, uint32_t* u32);
void eui_f32_edit(EditorTheme theme, const char* label, float* f32);
void eui_vec2_edit(EditorTheme theme, const char* label, Vec2* v);
void eui_vec3_edit(EditorTheme theme, const char* label, Vec3* v);

} // namespace LD