#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct EUIU32EditStorage
{
    UITextEditStorage u32Edit;

    void init(uint32_t u32);
};

bool eui_u32_edit(EUIU32EditStorage* storage, const char* label, uint32_t* u32);

struct EUIF32EditStorage
{
    UITextEditStorage f32Edit;

    void init(float f32);
};

bool eui_f32_edit(EUIF32EditStorage* storage, const char* label, float* f32);
void eui_vec2_edit(EditorTheme theme, const char* label, Vec2* v);
void eui_vec3_edit(EditorTheme theme, const char* label, Vec3* v);

} // namespace LD