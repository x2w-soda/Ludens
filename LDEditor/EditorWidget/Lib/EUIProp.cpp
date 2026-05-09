#include <Ludens/Header/Assert.h>
#include <Ludens/Serial/Property.h>
#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIProp.h>

#include <format>
#include <string>

#include "EUI.h"

namespace LD {

/*
void EUIU32Prop::init(uint32_t u32)
{
    mEdit.set_text(std::to_string(u32));
}

bool EUIU32Prop::update(const char* label, uint32_t* u32)
{
    EditorTheme theme = eui_get_theme();
    bool hasChanged = false;
    String str;

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
    String str;

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
    String str;

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
    String str;

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
    String str;

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
*/

bool eui_u32_prop(View label, uint32_t* u32)
{
    EditorTheme theme = eui_get_theme();
    String str;
    bool hasChanged = false;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        auto* edit = (UITextEditData*)ui_push_text_edit(nullptr).get_data();
        edit->set_domain(UI_TEXT_EDIT_DOMAIN_UINT);
        if (!ui_text_edit_is_editing())
            edit->set_text(std::format("{}", *u32));
        hasChanged = ui_text_edit_submitted(str);
        if (hasChanged)
            *u32 = (uint32_t)std::stoul(std::string(str.c_str()));
        ui_pop();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_f32_prop(View label, float* f32)
{
    EditorTheme theme = eui_get_theme();
    String str;
    bool hasChanged = false;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        hasChanged = push_text_edit_f32(nullptr, f32, str, false);
        ui_top_layout(layoutI);
        pop_text_edit_f32();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_vec2_prop(View label, Vec2* v)
{
    return eui_vec2_prop(label, (float*)v);
}

bool eui_vec2_prop(View label, float f32[2])
{
    EditorTheme theme = eui_get_theme();
    String str;
    bool hasChanged = false;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        for (int i = 0; i < 2; i++)
        {
            bool commit = push_text_edit_f32(nullptr, f32 + i, str, false);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_slider_prop(View label, float* f32)
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

        ui_push_slider(nullptr, f32);
        isDragged = ui_top_drag(btn, dragPos, dragBegin);
        ui_pop();
    }
    pop_prop_hbox();

    return isDragged;
}

bool eui_toggle_prop(View label, bool* b8)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        UIToggleData* toggle = (UIToggleData*)ui_push_toggle(nullptr).get_data();
        ui_top_layout_size(UISize::fixed(80.0f), layoutI.sizeY);
        if (ui_toggle_is_pressed())
        {
            hasChanged = true;
            *b8 = toggle->state;
        }
        else
            toggle->state = *b8;
        ui_pop();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_rect_prop(View label, Rect* rect, bool normalized)
{
    EditorTheme theme = eui_get_theme();
    bool hasChanged = false;
    String str;

    push_prop_hbox();
    {
        bool commit;
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        push_prop_edit_vbox();
        {
            commit = push_text_edit_f32(nullptr, &rect->x, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
            commit = push_text_edit_f32(nullptr, &rect->w, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
        pop_prop_edit_vbox();

        push_prop_edit_vbox();
        {
            commit = push_text_edit_f32(nullptr, &rect->y, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
            commit = push_text_edit_f32(nullptr, &rect->h, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
        pop_prop_edit_vbox();
    }
    pop_prop_hbox();

    return hasChanged;
}

Vector<PropertyDelta> eui_component_property_table(ComponentView& view)
{
    void* obj = view.data();
    const TypeMeta& typeM = *view.type_meta();
    EditorContext ctx = eui_get_context();
    EditorTheme theme = eui_get_theme();

    Vector<PropertyValue> oldProps = typeM.get_property_snapshot(obj);
    Vector<PropertyValue> newProps = oldProps;

    for (PropertyValue& prop : newProps)
    {
        const PropertyMeta& propM = typeM.props[prop.propIndex];

        switch (propM.valueType)
        {
        case VALUE_TYPE_F32:
            if (propM.uiHint == PROPERTY_UI_HINT_SLIDER)
                eui_slider_prop(propM.name, prop.value.v16.f32);
            else
                eui_f32_prop(propM.name, prop.value.v16.f32);
            break;
        case VALUE_TYPE_U32:
            if (propM.uiHint == PROPERTY_UI_HINT_ASSET)
            {
                constexpr uint32_t assetIndex = 0; // TODO:
                AssetType assetType = view.get_asset_type(assetIndex);
                AssetID assetID = (AssetID)prop.value.get_u32();
                if (eui_asset_slot(assetID, assetType))
                    EditorContextUtil::request_component_asset(ctx, view.suid(), assetID, assetType, assetIndex);
            }
            else
                eui_u32_prop(propM.name, prop.value.v16.u32);
            break;
        case VALUE_TYPE_BOOL:
            eui_toggle_prop(propM.name, prop.value.v16.b8);
            break;
        case VALUE_TYPE_VEC2:
            eui_vec2_prop(propM.name, (float*)&prop.value.v16.f32);
            break;
        case VALUE_TYPE_RECT:
            eui_rect_prop(propM.name, &prop.value.v16.rect, propM.flags & PROPERTY_FLAG_NORMALIZED_BIT);
            break;
        case VALUE_TYPE_TRANSFORM_2D:
        {
            eui_vec2_prop("Position", &prop.value.transform2D.position);
            eui_f32_prop("Rotation", &prop.value.transform2D.rotation);
            eui_vec2_prop("Scale", &prop.value.transform2D.scale);
            break;
        }
        default:
            LD_DEBUG_BREAK;
            break;
        }
    }

    return typeM.get_property_delta(obj, oldProps, newProps);
}

} // namespace LD