#include <LudensEditor/EditorWidget/UIDraw.h>
#include <LudensEditor/EditorWidget/UIVectorEditWidget.h>

#include <format>
#include <string>

#define PANEL_CHILD_GAP 6.0f
#define PANEL_CHILD_PAD 2.0f

namespace LD {

void eui_u32_edit(EditorTheme theme, const char* label, uint32_t* u32)
{
    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel();
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{}", *u32);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
        ui_pop();
    }
    ui_pop();
}

void eui_f32_edit(EditorTheme theme, const char* label, float* f32)
{
    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel();
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{:8.3f}", *f32);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
        ui_pop();
    }
    ui_pop();
}

void eui_vec2_edit(EditorTheme theme, const char* label, Vec2* v)
{
    float childGap = PANEL_CHILD_GAP;
    float childPad = PANEL_CHILD_PAD;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap, &childPad);
    layoutI.sizeX = UISize::grow();
    ui_push_panel();
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{:8.3f}", v->x);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
        ui_pop();

        str = std::format("{:8.3f}", v->y);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
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
    ui_push_panel();
    ui_top_layout(layoutI);
    {
        layoutI = theme.make_text_label_layout();
        ui_push_text(label);
        ui_top_layout(layoutI);
        ui_pop();

        std::string str;
        str = std::format("{:8.3f}", v->x);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
        ui_pop();

        str = std::format("{:8.3f}", v->y);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
        ui_pop();

        str = std::format("{:8.3f}", v->z);
        ui_push_text(str.c_str());
        ui_top_layout(layoutI);
        ui_top_draw(&eui_draw_text_with_bg);
        ui_pop();
    }
    ui_pop();
}

} // namespace LD