#include <Ludens/Header/Assert.h>
#include <LudensEditor/EditorWidget/EUIVectorEdit.h>

#include <format>
#include <string>

#include "EUI.h"

namespace LD {

void EUIU32Storage::init(uint32_t u32)
{
    LD_ASSERT(u32Edit.editor);
    u32Edit.editor.set_string(std::to_string(u32));
}

void EUIF32Storage::init(float f32)
{
    LD_ASSERT(f32Edit.editor);
    f32Edit.editor.set_string(std::to_string(f32));
}

void EUIVec2Storage::init(const Vec2& vec2)
{
    vec2Edit[0].editor.set_string(std::to_string(vec2.x));
    vec2Edit[1].editor.set_string(std::to_string(vec2.y));
}

bool eui_u32_edit(EUIU32Storage* storage, const char* label, uint32_t* u32)
{
    LD_ASSERT(storage);

    EditorTheme theme = eui_get_theme();
    bool hasChanged = false;
    std::string str;

    UILayoutInfo layoutI = theme.make_hbox_layout();
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
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
    ui_pop();

    return hasChanged;
}

bool eui_f32_edit(EUIF32Storage* storage, const char* label, float* f32)
{
    LD_ASSERT(storage);

    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
    std::string str;

    UILayoutInfo layoutI = theme.make_hbox_layout();
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        edit = ui_push_text_edit(&storage->f32Edit);
        edit->set_domain(UI_TEXT_EDIT_DOMAIN_UINT);
        if (!ui_text_edit_is_editing())
            edit->set_text(std::format("{:8.3f}", *f32));
        if (ui_text_edit_submitted(str))
        {
            *f32 = std::stof(str);
            hasChanged = true;
        }

        ui_top_layout(layoutI);
        ui_pop();
    }
    ui_pop();

    return hasChanged;
}

bool eui_vec2_edit(EUIVec2Storage* storage, const char* label, Vec2* v)
{
    LD_ASSERT(storage);

    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
    std::string str;

    UILayoutInfo layoutI = theme.make_hbox_layout();
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        for (int i = 0; i < 2; i++)
        {
            float* f32 = &v->x + i;
            edit = ui_push_text_edit(&storage->vec2Edit[i]);
            edit->set_domain(UI_TEXT_EDIT_DOMAIN_F32);
            if (!ui_text_edit_is_editing())
                edit->set_text(std::format("{:8.3f}", *f32));
            if (ui_text_edit_submitted(str))
            {
                *f32 = std::stof(str);
                hasChanged = true;
            }

            ui_top_layout(layoutI);
            ui_pop();
        }
    }
    ui_pop();

    return hasChanged;
}

bool eui_vec3_edit(EUIVec3Storage* storage, const char* label, Vec3* v)
{
    bool hasChanged = false;
    EditorTheme theme = eui_get_theme();
    UITextEditStorage* edit;
    std::string str;

    UILayoutInfo layoutI = theme.make_hbox_layout();
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        for (int i = 0; i < 3; i++)
        {
            float* f32 = &v->x + i;
            edit = ui_push_text_edit(&storage->vec3Edit[i]);
            edit->set_domain(UI_TEXT_EDIT_DOMAIN_F32);
            if (!ui_text_edit_is_editing())
                edit->set_text(std::format("{:8.3f}", *f32));
            if (ui_text_edit_submitted(str))
            {
                *f32 = std::stof(str);
                hasChanged = true;
            }

            ui_top_layout(layoutI);
            ui_pop();
        }
    }
    ui_pop();

    return hasChanged;
}

bool eui_slider_edit(EUISliderStorage* storage, const char* label, float* f32)
{
    bool isDragged;
    EditorTheme theme = eui_get_theme();
    UILayoutInfo layoutI{};
    MouseButton btn;
    Vec2 dragPos;
    bool dragBegin;

    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_child_gap();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        ui_push_text(nullptr, label);
        ui_top_layout(theme.make_text_label_layout());
        ui_pop();

        ui_push_slider(&storage->slider, f32);
        isDragged = ui_top_drag(btn, dragPos, dragBegin);
        ui_pop();
    }
    ui_pop();

    return isDragged;
}

} // namespace LD