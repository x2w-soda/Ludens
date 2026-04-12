#include <Ludens/Header/Assert.h>
#include <LudensEditor/EditorWidget/EUIPrimitiveEdit.h>

#include <format>
#include <string>

#include "EUI.h"

namespace LD {

void EUIU32Storage::init(uint32_t u32)
{
    u32Edit.set_text(std::to_string(u32));
}

void EUIF32Storage::init(float f32)
{
    f32Edit.set_text(std::to_string(f32));
}

void EUIVec2Storage::init(const Vec2& vec2)
{
    vec2Edit[0].set_text(std::to_string(vec2.x));
    vec2Edit[1].set_text(std::to_string(vec2.y));
}

void EUIRectStorage::init(Rect rect)
{
    rectEdit[0].set_text(std::to_string(rect.x));
    rectEdit[1].set_text(std::to_string(rect.y));
    rectEdit[2].set_text(std::to_string(rect.w));
    rectEdit[3].set_text(std::to_string(rect.h));
}

bool eui_u32_edit(EUIU32Storage* storage, const char* label, uint32_t* u32)
{
    LD_ASSERT(storage);

    EditorTheme theme = eui_get_theme();
    bool hasChanged = false;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        UITextEditStorage* edit = &storage->u32Edit;
        edit = ui_push_text_edit(edit);
        edit->set_domain(UI_TEXT_EDIT_DOMAIN_UINT);
        if (!ui_text_edit_is_editing())
            edit->set_text(std::format("{}", *u32));
        hasChanged = ui_text_edit_submitted(str);
        if (hasChanged)
            *u32 = (uint32_t)std::stoul(str);

        ui_top_layout(layoutI);
        ui_pop();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_f32_edit(EUIF32Storage* storage, const char* label, float* f32)
{
    LD_ASSERT(storage);

    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        hasChanged = push_text_edit_f32(&storage->f32Edit, f32, str, false);
        ui_top_layout(layoutI);
        pop_text_edit_f32();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_vec2_edit(EUIVec2Storage* storage, const char* label, Vec2* v)
{
    LD_ASSERT(storage);

    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
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
            bool commit = push_text_edit_f32(&storage->vec2Edit[i], f32, str, false);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_vec3_edit(EUIVec3Storage* storage, const char* label, Vec3* v)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
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
            bool commit = push_text_edit_f32(&storage->vec3Edit[i], f32, str, false);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_slider_edit(EUISliderStorage* storage, const char* label, float* f32)
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

        ui_push_slider(&storage->slider, f32);
        isDragged = ui_top_drag(btn, dragPos, dragBegin);
        ui_pop();
    }
    pop_prop_hbox();

    return isDragged;
}

bool eui_rect_edit(EUIRectStorage* storage, const char* label, Rect* rect, bool normalized)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
    std::string str;

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        push_prop_edit_vbox();
        {
            bool commit = push_text_edit_f32(&storage->rectEdit[0], &rect->x, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
            commit = push_text_edit_f32(&storage->rectEdit[2], &rect->w, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
        pop_prop_edit_vbox();

        push_prop_edit_vbox();
        {
            bool commit = push_text_edit_f32(&storage->rectEdit[1], &rect->y, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
            commit = push_text_edit_f32(&storage->rectEdit[3], &rect->h, str, normalized);
            hasChanged = hasChanged || commit;
            ui_top_layout(layoutI);
            pop_text_edit_f32();
        }
        pop_prop_edit_vbox();
    }
    pop_prop_hbox();

    return hasChanged;
}

bool eui_toggle_edit(EUIToggleStorage* storage, const char* label, bool* state)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();

    push_prop_hbox();
    {
        UILayoutInfo layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        UIToggleStorage* toggle = ui_push_toggle(&storage->toggle);
        ui_top_layout_size(UISize::fixed(80.0f), layoutI.sizeY);
        if (ui_toggle_is_pressed())
        {
            hasChanged = true;
            *state = toggle->state;
        }
        else
            toggle->state = *state;
        ui_pop();
    }
    pop_prop_hbox();

    return hasChanged;
}

} // namespace LD