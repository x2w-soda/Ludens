#pragma once
#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/UI/UIWindowManager.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EOutlinerWindowInfo
{
    EditorContext ctx;
    UIWindowManager wm;    /// window manager handle
    UIWindowAreaID areaID; /// designated window area
};

/// @brief Editor outliner window. Displays the scene hierarchy.
struct EOutlinerWindow : Handle<struct EOutlinerWindowObj>
{
    /// @brief Create editor outliner window.
    /// @param windowInfo Outliner window creation info.
    /// @return Editor outliner handle.
    static EOutlinerWindow create(const EOutlinerWindowInfo& windowInfo);

    /// @brief Destroy editor outliner window
    static void destroy(EOutlinerWindow window);
};

} // namespace LD