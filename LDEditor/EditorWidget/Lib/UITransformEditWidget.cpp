#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>
#include <LudensEditor/EditorWidget/UIDraw.h>

#include <format>

#define PANEL_CHILD_GAP 6.0f

namespace LD {

static void eui_transform_edit_vec3(const char* label, const Vec3& v, float fontSize);
static void eui_transform_2d_edit_vec2(const char* label, const Vec2& v, float fontSize);
static void eui_transform_2d_edit_float(const char* label, float f32, float fontSize);

static void eui_transform_edit_vec3(const char* label, const Vec3& v, float fontSize)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = PANEL_CHILD_GAP;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(fontSize * 1.2f);
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

static void eui_transform_2d_edit_vec2(const char* label, const Vec2& v, float fontSize)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = PANEL_CHILD_GAP;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(fontSize * 1.2f);
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

static void eui_transform_2d_edit_float(const char* label, float f32, float fontSize)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = PANEL_CHILD_GAP;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(fontSize * 1.2f);
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

void eui_transform_edit(EditorTheme editorTheme, TransformEx* transform)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    float fontSize = editorTheme.get_font_size();
    eui_transform_edit_vec3("Position", transform->position, fontSize);
    eui_transform_edit_vec3("Rotation", transform->rotationEuler, fontSize);
    eui_transform_edit_vec3("Scale", transform->scale, fontSize);

    ui_pop();
}

void eui_transform_2d_edit(EditorTheme editorTheme, Transform2D* transform2D)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    float fontSize = editorTheme.get_font_size();
    eui_transform_2d_edit_vec2("Position", transform2D->position, fontSize);
    eui_transform_2d_edit_vec2("Scale", transform2D->scale, fontSize);
    eui_transform_2d_edit_float("Rotation", transform2D->rotation, fontSize);

    ui_pop();
}

} // namespace LD
