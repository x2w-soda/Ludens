#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorWidget/EUIPrimitiveEdit.h>
#include <LudensEditor/EditorWidget/EUITransformEdit.h>

#include "EUI.h"

#include <format>

namespace LD {

bool eui_transform_edit(EUITransformStorage* storage, TransformEx* transform)
{
    LD_ASSERT(storage);

    bool hasChanged = false;
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    if (eui_vec3_edit(&storage->position, "Position", &transform->position))
        hasChanged = true;

    if (eui_vec3_edit(&storage->rotation, "Rotation", &transform->rotationEuler))
        hasChanged = true;

    if (eui_vec3_edit(&storage->scale, "Scale", &transform->scale))
        hasChanged = true;

    ui_pop();

    return hasChanged;
}

bool eui_transform_2d_edit(EUITransform2DStorage* storage, Transform2D* transform2D)
{
    bool hasChanged = false;
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    if (eui_vec2_edit(&storage->position, "Position", &transform2D->position))
        hasChanged = true;

    if (eui_f32_edit(&storage->rotation, "Rotation", &transform2D->rotation))
        hasChanged = true;

    if (eui_vec2_edit(&storage->scale, "Scale", &transform2D->scale))
        hasChanged = true;

    ui_pop();

    return hasChanged;
}

} // namespace LD
