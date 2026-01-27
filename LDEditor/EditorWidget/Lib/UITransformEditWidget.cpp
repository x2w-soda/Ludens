#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>
#include <LudensEditor/EditorWidget/UIDraw.h>

#include <format>

#define PANEL_CHILD_GAP 6.0f

namespace LD {

static void eui_transform_edit_vec3(EditorTheme theme, const char* label, const Vec3& v);
static void eui_transform_2d_edit_vec2(EditorTheme theme, const char* label, const Vec2& v);
static void eui_transform_2d_edit_float(EditorTheme theme, const char* label, float f32);

static void eui_transform_edit_vec3(EditorTheme theme, const char* label, const Vec3& v)
{
    const float textLabelWidth = theme.get_text_label_width();
    const float textRowHeight = theme.get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = PANEL_CHILD_GAP;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI.sizeX = UISize::fixed(textLabelWidth);
    layoutI.sizeY = UISize::fixed(textRowHeight);
    ui_push_text(label);
    ui_top_layout(layoutI);
    ui_pop();

    std::string str;
    str = std::format("{:8.3f}", v.x);
    ui_push_text(str.c_str());
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    ui_pop();

    str = std::format("{:8.3f}", v.y);
    ui_push_text(str.c_str());
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    ui_pop();

    str = std::format("{:8.3f}", v.z);
    ui_push_text(str.c_str());
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    ui_pop();

    ui_pop();
}

static void eui_transform_2d_edit_vec2(EditorTheme theme, const char* label, const Vec2& v)
{
    const float textLabelWidth = theme.get_text_label_width();
    const float textRowHeight = theme.get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = PANEL_CHILD_GAP;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI.sizeX = UISize::fixed(textLabelWidth);
    layoutI.sizeY = UISize::fixed(textRowHeight);
    ui_push_text(label);
    ui_top_layout(layoutI);
    ui_pop();

    std::string str;
    str = std::format("{:8.3f}", v.x);
    ui_push_text(str.c_str());
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    ui_pop();

    str = std::format("{:8.3f}", v.y);
    ui_push_text(str.c_str());
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    ui_pop();

    ui_pop();
}

static void eui_transform_2d_edit_float(EditorTheme theme, const char* label, float f32)
{
    const float textLabelWidth = theme.get_text_label_width();
    const float textRowHeight = theme.get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = PANEL_CHILD_GAP;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI.sizeX = UISize::fixed(textLabelWidth);
    layoutI.sizeY = UISize::fixed(textRowHeight);
    ui_push_text(label);
    ui_top_layout(layoutI);
    ui_pop();

    std::string str;
    str = std::format("{:8.3f}", f32);
    ui_push_text(str.c_str());
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    ui_pop();

    ui_pop();
}

void eui_transform_edit(EditorTheme theme, TransformEx* transform)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    float fontSize = theme.get_font_size();
    eui_transform_edit_vec3(theme, "Position", transform->position);
    eui_transform_edit_vec3(theme, "Rotation", transform->rotationEuler);
    eui_transform_edit_vec3(theme, "Scale", transform->scale);

    ui_pop();
}

void eui_transform_2d_edit(EditorTheme theme, Transform2D* transform2D)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    eui_transform_2d_edit_vec2(theme, "Position", transform2D->position);
    eui_transform_2d_edit_vec2(theme, "Scale", transform2D->scale);
    eui_transform_2d_edit_float(theme, "Rotation", transform2D->rotation);

    ui_pop();
}

} // namespace LD
