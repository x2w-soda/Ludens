#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

enum ProjectWindowMode : EditorWindowMode
{
    PROJECT_WINDOW_MODE_SELECT_PROJECT,
    PROJECT_WINDOW_MODE_CREATE_PROJECT,
    PROJECT_WINDOW_MODE_SAVE_PROJECT,
    PROJECT_WINDOW_MODE_CREATE_SCENE,
};

/// @brief Window to create new project or open existing project.
struct ProjectWindow : Handle<struct ProjectWindowObj>
{
    ProjectWindow() = default;
    ProjectWindow(const EditorWindowObj* obj) { mObj = (ProjectWindowObj*)obj; }

    void set_mode(ProjectWindowMode mode);
    void set_save_project_continuation(EditorWindowType windowType, EditorWindowMode modeHint);

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
    static void mode_hint(EditorWindowObj* obj, EditorWindowMode modeHint);
};

} // namespace LD
