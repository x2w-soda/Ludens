#include <Ludens/Header/Assert.h>
#include <LudensEditor/EditorWidget/EUIProp.h>

#include <format>
#include <string>

#include "EUI.h"

namespace LD {

void EUIU32Prop::init(uint32_t u32)
{
    mEdit.set_text(std::to_string(u32));
}

bool EUIU32Prop::update(const char* label, uint32_t* u32)
{
    EditorTheme theme = eui_get_theme();
    bool hasChanged = false;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        ui_push_text_edit(&mEdit);
        mEdit.set_domain(UI_TEXT_EDIT_DOMAIN_UINT);
        if (!ui_text_edit_is_editing())
            mEdit.set_text(std::format("{}", *u32));
        hasChanged = ui_text_edit_submitted(str);
        if (hasChanged)
            *u32 = (uint32_t)std::stoul(str);

        ui_top_layout(layoutI);
        ui_pop();
    }
    pop_prop_hbox();

    return hasChanged;
}

void EUIF32Prop::init(float f32)
{
    mEdit.set_text(std::to_string(f32));
}

bool EUIF32Prop::update(const char* label, float* f32)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        hasChanged = push_text_edit_f32(&mEdit, f32, str, false);
        ui_top_layout(layoutI);
        pop_text_edit_f32();
    }
    pop_prop_hbox();

    return hasChanged;
}

void EUIVec2Prop::init(Vec2 vec2)
{
    mEdit[0].set_text(std::to_string(vec2.x));
    mEdit[1].set_text(std::to_string(vec2.y));
}

bool EUIVec2Prop::update(const char* label, Vec2* v)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditData* edit;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        for (int i = 0; i < 2; i++)
        {
            float* f32 = &v->x + i;
            bool commit = push_text_edit_f32(&mEdit[i], f32, str, false);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
    }
    pop_prop_hbox();

    return hasChanged;
}

void EUIRectProp::init(Rect rect)
{
    mEdit[0].set_text(std::to_string(rect.x));
    mEdit[1].set_text(std::to_string(rect.y));
    mEdit[2].set_text(std::to_string(rect.w));
    mEdit[3].set_text(std::to_string(rect.h));
}

bool EUIRectProp::update(const char* label, Rect* rect, bool normalized)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditData* edit;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        push_prop_edit_vbox();
        {
            bool commit = push_text_edit_f32(&mEdit[0], &rect->x, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
            commit = push_text_edit_f32(&mEdit[2], &rect->w, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
        pop_prop_edit_vbox();

        push_prop_edit_vbox();
        {
            bool commit = push_text_edit_f32(&mEdit[1], &rect->y, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
            commit = push_text_edit_f32(&mEdit[3], &rect->h, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
        pop_prop_edit_vbox();
    }
    pop_prop_hbox();

    return hasChanged;
}

void EUIVec3Prop::init(Vec3 vec3)
{
    mEdit[0].set_text(std::to_string(vec3.x));
    mEdit[1].set_text(std::to_string(vec3.y));
    mEdit[2].set_text(std::to_string(vec3.z));
}

bool EUIVec3Prop::update(const char* label, Vec3* v)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditData* edit;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        for (int i = 0; i < 3; i++)
        {
            float* f32 = &v->x + i;
            bool commit = push_text_edit_f32(&mEdit[i], f32, str, false);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
    }
    pop_prop_hbox();

    return hasChanged;
}

bool EUISliderProp::update(const char* label, float* f32)
{
    bool isDragged;
    EditorTheme theme = eui_get_theme();
    MouseButton btn;
    Vec2 dragPos;
    bool dragBegin;

    push_prop_hbox();
    {
        ui_push_text(nullptr, label);
        ui_top_layout(theme.make_text_label_layout());
        ui_pop();

        ui_push_slider(&mSlider, f32);
        isDragged = ui_top_drag(btn, dragPos, dragBegin);
        ui_pop();
    }
    pop_prop_hbox();

    return isDragged;
}

bool EUIToggleProp::update(const char* label, bool* state)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        ui_push_toggle(&mToggle);
        ui_top_layout_size(UISize::fixed(80.0f), layoutI.sizeY);
        if (ui_toggle_is_pressed())
        {
            hasChanged = true;
            *state = mToggle.state;
        }
        else
            mToggle.state = *state;
        ui_pop();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool EUIButtonProp::update(const char* btnText, bool isEnabled)
{
    bool isPressed = false;
    EditorTheme theme = eui_get_theme();

    mButton.isEnabled = isEnabled;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr);
        ui_top_layout(layoutI);
        ui_pop();

        ui_push_button(&mButton, btnText);
        if (isEnabled && ui_button_is_pressed())
            isPressed = true;
        ui_pop();
    }
    pop_prop_hbox();

    return isPressed;
}

bool EUITransformProp::update(TransformEx* transform)
{
    bool hasChanged = false;
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    if (mPos.update("Position", &transform->position))
        hasChanged = true;

    if (mRot.update("Rotation", &transform->rotationEuler))
        hasChanged = true;

    if (mScale.update("Scale", &transform->scale))
        hasChanged = true;

    ui_pop();

    return hasChanged;
}

bool EUITransform2DProp::update(Transform2D* transform2D)
{
    bool hasChanged = false;
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    if (mPos.update("Position", &transform2D->position))
        hasChanged = true;

    if (mRot.update("Rotation", &transform2D->rotation))
        hasChanged = true;

    if (mScale.update("Scale", &transform2D->scale))
        hasChanged = true;

    ui_pop();

    return hasChanged;
}

} // namespace LD