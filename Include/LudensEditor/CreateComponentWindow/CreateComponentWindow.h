#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Window to create a new component.
struct CreateComponentWindow : Handle<struct CreateComponentWindowObj>
{
    CreateComponentWindow() = default;
    CreateComponentWindow(const EditorWindowObj* obj) { mObj = (CreateComponentWindowObj*)obj; }

    /// @brief Create editor selection window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy editor selection window.
    static void destroy(EditorWindow window);

    /// @brief Set the parent component before creation.
    void set_parent_component(CUID parentID);
};

} // namespace LD
