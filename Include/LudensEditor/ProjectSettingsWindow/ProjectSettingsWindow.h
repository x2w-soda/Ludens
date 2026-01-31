#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor selection window. Generic dialog for all kinds of selection tasks.
struct ProjectSettingsWindow : Handle<struct ProjectSettingsWindowObj>
{
    ProjectSettingsWindow() = default;
    ProjectSettingsWindow(const EditorWindowObj* obj) { mObj = (ProjectSettingsWindowObj*)obj; }

    /// @brief Create editor project settings window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy editor project settings window.
    static void destroy(EditorWindow window);
};

} // namespace LD