#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct EUIU32Storage
{
    UITextEditData u32Edit;

    void init(uint32_t u32);
};

bool eui_u32_edit(EUIU32Storage* storage, const char* label, uint32_t* u32);

struct EUIF32Storage
{
    UITextEditData f32Edit;

    void init(float f32);
};

bool eui_f32_edit(EUIF32Storage* storage, const char* label, float* f32);

struct EUIVec2Storage
{
    UITextEditData vec2Edit[2];

    void init(const Vec2& vec2);
};

bool eui_vec2_edit(EUIVec2Storage* storage, const char* label, Vec2* v);

struct EUIVec3Storage
{
    UITextEditData vec3Edit[3];

    void init(const Vec2& vec3);
};

bool eui_vec3_edit(EUIVec3Storage* storage, const char* label, Vec3* v);

struct EUISliderStorage
{
    UISliderData slider;
};

bool eui_slider_edit(EUISliderStorage* storage, const char* label, float* f32);

struct EUIRectStorage
{
    UITextEditData rectEdit[4];

    void init(Rect rect);
};

bool eui_rect_edit(EUIRectStorage* storage, const char* label, Rect* rect, bool normalized);

struct EUIToggleStorage
{
    UIToggleData toggle;
};

bool eui_toggle_edit(EUIToggleStorage* storage, const char* label, bool* state);

} // namespace LD