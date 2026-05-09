#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Serial/Property.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

class ComponentView;
struct Transform;
struct TransformEx;
struct Transform2D;

/*
class EUIU32Prop
{
public:
    void init(uint32_t u32);
    bool update(const char* label, uint32_t* u32);

private:
    UITextEditData mEdit;
};

class EUIF32Prop
{
public:
    void init(float f32);
    bool update(const char* label, float* f32);

private:
    UITextEditData mEdit;
};

class EUIVec2Prop
{
public:
    void init(Vec2 vec2);
    bool update(const char* label, Vec2* v);

private:
    UITextEditData mEdit[2];
};

class EUIVec3Prop
{
public:
    void init(Vec3 vec3);
    bool update(const char* label, Vec3* v);

private:
    UITextEditData mEdit[3];
};

class EUISliderProp
{
public:
    bool update(const char* label, float* f32);

private:
    UISliderData mSlider;
};

class EUIRectProp
{
public:
    void init(Rect rect);
    bool update(const char* label, Rect* rect, bool normalized);

private:
    UITextEditData mEdit[4];
};

bool eui_rect_prop(const char* label, Rect* rect, bool normalized);

class EUIToggleProp
{
public:
    bool update(const char* label, bool* state);

private:
    UIToggleData mToggle;
};

class EUIButtonProp
{
public:
    bool update(const char* btnText, bool isEnabled = true);

private:
    UIButtonData mButton;
};

class EUITransformProp
{
public:
    bool update(TransformEx* transform);

private:
    EUIVec3Prop mPos;
    EUIVec3Prop mRot;
    EUIVec3Prop mScale;
};

class EUITransform2DProp
{
public:
    bool update(Transform2D* transform2D);

private:
    EUIVec2Prop mPos;
    EUIF32Prop mRot;
    EUIVec2Prop mScale;
};
*/

bool eui_u32_prop(View label, uint32_t* u32);
bool eui_f32_prop(View label, float* f32);
bool eui_vec2_prop(View label, Vec2* v);
bool eui_vec2_prop(View label, float f32[2]);
bool eui_slider_prop(View label, float* f32);
bool eui_toggle_prop(View label, bool* b8);
Vector<PropertyDelta> eui_component_property_table(ComponentView& view);

} // namespace LD