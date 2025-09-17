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

/// @brief Widget for inspecting and editing a Transform
struct UITransformEditWidget : Handle<struct UITransformEditWidgetObj>
{
    /// @brief Create widget.
    static UITransformEditWidget create(const UITransformEditWidgetInfo& info);

    /// @brief Destroy widget.
    static void destroy(UITransformEditWidget widget);

    /// @brief Set the transform address to edit.
    void set(Transform* transform);

    /// @brief Show widget subtree.
    void show();
    
    /// @brief Hide widget subtree.
    void hide();
};

} // namespace LD