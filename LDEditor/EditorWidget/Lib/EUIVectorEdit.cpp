#include <Ludens/Header/Assert.h>
#include <LudensEditor/EditorWidget/EUIVectorEdit.h>

#include <format>
#include <string>

#include "EUI.h"

#define PANEL_CHILD_GAP 6.0f
#define PANEL_CHILD_PAD 2.0f

namespace LD {

void EUIU32EditStorage::init(uint32_t u32)
{
    LD_ASSERT(u32Edit.buf);
    u32Edit.buf.set_string(std::to_string(u32));
}

void EUIF32EditStorage::init(float f32)
{
    LD_ASSERT(f32Edit.buf);
    f32Edit.buf.set_string(std::to_string(f32));
}

bool eui_u32_edit(EUIU32EditStorage* storage, const char* label, uint32_t* u32)
{
    LD_ASSERT(storage);

    EditorTheme theme = eui_get_theme();
    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;
    bool hasChanged = false;
    std::string str;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        UITextEditStorage* edit = storage ? &storage->u32Edit : nullptr;
        edit = ui_push_text_edit(edit);
        edit->domain = UI_TEXT_EDIT_DOMAIN_UINT;
        hasChanged = ui_text_edit_submitted(str);
        if (hasChanged)
            *u32 = (uint32_t)std::stoul(str);

        ui_top_layout(layoutI);
        ui_pop();
    }
    ui_pop();

    return hasChanged;
}

bool eui_f32_edit(EUIF32EditStorage* storage, const char* label, float* f32)
{
    LD_ASSERT(storage);

    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;
    EditorTheme theme = eui_get_theme();
    bool hasChanged = false;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{:8.3f}", *f32);
        ui_push_text(nullptr, str.c_str());
        ui_top_layout(layoutI);
        ui_pop();
    }
    ui_pop();

    return hasChanged;
}

void eui_vec2_edit(EditorTheme theme, const char* label, Vec2* v)
{
    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{:8.3f}", v->x);
        ui_push_text(nullptr, str.c_str());
        ui_top_layout(layoutI);
        ui_pop();

        str = std::format("{:8.3f}", v->y);
        ui_push_text(nullptr, str.c_str());
        ui_top_layout(layoutI);
        ui_pop();
    }
    ui_pop();
}

void eui_vec3_edit(EditorTheme theme, const char* label, Vec3* v)
{
    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(nullptr, label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{:8.3f}", v->x);
        ui_push_text(nullptr, str.c_str());
        ui_top_layout(layoutI);
        ui_pop();

        str = std::format("{:8.3f}", v->y);
        ui_push_text(nullptr, str.c_str());
        ui_top_layout(layoutI);
        ui_pop();

        str = std::format("{:8.3f}", v->z);
        ui_push_text(nullptr, str.c_str());
        ui_top_layout(layoutI);
        ui_pop();
    }
    ui_pop();
}

} // namespace LD