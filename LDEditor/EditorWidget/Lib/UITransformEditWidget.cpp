#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorWidget/UIDraw.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>
#include <LudensEditor/EditorWidget/UIVectorEditWidget.h>

#include <format>

namespace LD {

void eui_transform_edit(EditorTheme theme, TransformEx* transform)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    float fontSize = theme.get_font_size();
    eui_vec3_edit(theme, "Position", &transform->position);
    eui_vec3_edit(theme, "Rotation", &transform->rotationEuler);
    eui_vec3_edit(theme, "Scale", &transform->scale);

    ui_pop();
}

void eui_transform_2d_edit(EditorTheme theme, Transform2D* transform2D)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    eui_vec2_edit(theme, "Position", &transform2D->position);
    eui_vec2_edit(theme, "Scale", &transform2D->scale);
    eui_f32_edit(theme, "Rotation", &transform2D->rotation);

    ui_pop();
}

} // namespace LD
