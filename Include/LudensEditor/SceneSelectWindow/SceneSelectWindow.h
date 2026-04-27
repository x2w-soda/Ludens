#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Window to select a Scene from a Project.
struct SceneSelectWindow : Handle<struct SceneSelectWindowObj>
{
    SceneSelectWindow() = default;
    SceneSelectWindow(const EditorWindowObj* obj) { mObj = (SceneSelectWindowObj*)obj; }

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
};

} // namespace LD
