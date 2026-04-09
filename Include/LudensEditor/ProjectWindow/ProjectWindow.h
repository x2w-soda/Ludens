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

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
};

} // namespace LD