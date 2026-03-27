#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Window to create new project or open existing project.
struct ProjectWindow : Handle<struct ProjectWindowObj>
{
    ProjectWindow() = default;
    ProjectWindow(const EditorWindowObj* obj) { mObj = (ProjectWindowObj*)obj; }

    /// @brief Create editor project window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy editor project window.
    static void destroy(EditorWindow window);
};

} // namespace LD