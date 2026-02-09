#pragma once
#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIWindow.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor outliner window. Displays the scene hierarchy.
struct OutlinerWindow : Handle<struct OutlinerWindowObj>
{
    OutlinerWindow() = default;
    OutlinerWindow(const EditorWindowObj* obj) { mObj = (OutlinerWindowObj*)obj; }

    /// @brief Create editor outliner window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy editor outliner window
    static void destroy(EditorWindow window);
};

} // namespace LD