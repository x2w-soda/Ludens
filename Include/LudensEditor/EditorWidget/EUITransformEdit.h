#pragma once

#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorWidget/EUIVectorEdit.h>

namespace LD {

struct Transform;
struct TransformEx;
struct Transform2D;

struct EUITransformStorage
{
    EUIVec3Storage position;
    EUIVec3Storage rotation;
    EUIVec3Storage scale;
};

bool eui_transform_edit(EUITransformStorage* storage, TransformEx* transform);

struct EUITransform2DStorage
{
    EUIVec2Storage position;
    EUIF32Storage rotation;
    EUIVec2Storage scale;
};

bool eui_transform_2d_edit(EUITransform2DStorage* storage, Transform2D* transform2D);

} // namespace LD