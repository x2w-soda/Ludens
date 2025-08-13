#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/UI/UIWindowManager.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EInspectorWindowInfo
{
    EditorContext ctx;     /// editor context handle
    UIWindowManager wm;    /// window manager handle
    UIWindowAreaID areaID; /// designated window area
};

/// @brief Editor inspector window.
///        Displays the properties of the selected object.
struct EInspectorWindow : Handle<struct EInspectorWindowObj>
{
    static EInspectorWindow create(const EInspectorWindowInfo& windowInfo);
    static void destroy(EInspectorWindow window);
};

} // namespace LD