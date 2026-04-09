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

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);

    /// @brief Set the parent component before creation.
    void set_parent_component(SUID parentSUID);
};

} // namespace LD
