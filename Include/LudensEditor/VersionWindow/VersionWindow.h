#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorSettings.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Displays engine version.
struct VersionWindow : Handle<struct VersionWindowObj>
{
    VersionWindow() = default;
    VersionWindow(const EditorWindowObj* obj) { mObj = (VersionWindowObj*)obj; }

    /// @brief Create version info window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy version info window.
    static void destroy(EditorWindow window);

    void show();
};

} // namespace LD