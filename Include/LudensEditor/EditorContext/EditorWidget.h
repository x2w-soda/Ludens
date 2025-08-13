#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct Transform;

struct UITransformEditWidgetInfo
{
    EditorTheme theme;
    UIWidget parent;
};

/// @brief Edit and inspect a Transform
struct UITransformEditWidget : Handle<struct UITransformEditWidgetObj>
{
    static UITransformEditWidget create(const UITransformEditWidgetInfo& info);

    void set(Transform* transform);

    void on_draw(ScreenRenderComponent renderer);
};

} // namespace LD