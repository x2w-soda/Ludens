#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>
#include <LudensEditor/EditorWidget/UIDraw.h>
#include <format>

#define PANEL_CHILD_GAP 6.0f

namespace LD {

static void eui_transform_edit_row(const char* label, const Vec3& v, float fontSize);

void eui_transform_edit(EditorTheme editorTheme, Transform* transform)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    float fontSize = editorTheme.get_font_size();
    eui_transform_edit_row("Position", transform->position, fontSize);
    eui_transform_edit_row("Rotation", transform->rotation, fontSize);
    eui_transform_edit_row("Scale", transform->scale, fontSize);

    ui_pop();
}

static void eui_transform_edit_row(const char* label, const Vec3& v, float fontSize)
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

} // namespace LD
