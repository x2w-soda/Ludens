#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct EUIU32Storage
{
    UITextEditStorage u32Edit;

    void init(uint32_t u32);
};

bool eui_u32_edit(EUIU32Storage* storage, const char* label, uint32_t* u32);

struct EUIF32Storage
{
    UITextEditStorage f32Edit;

    void init(float f32);
};

bool eui_f32_edit(EUIF32Storage* storage, const char* label, float* f32);

struct EUIVec2Storage
{
    UITextEditStorage vec2Edit[2];

    void init(const Vec2& vec2);
};

bool eui_vec2_edit(EUIVec2Storage* storage, const char* label, Vec2* v);

struct EUIVec3Storage
{
    UITextEditStorage vec3Edit[3];

    void init(const Vec2& vec3);
};

bool eui_vec3_edit(EUIVec3Storage* storage, const char* label, Vec3* v);

struct EUISliderStorage
{
    UISliderStorage slider;
};

bool eui_slider_edit(EUISliderStorage* storage, const char* label, float* f32);

} // namespace LD