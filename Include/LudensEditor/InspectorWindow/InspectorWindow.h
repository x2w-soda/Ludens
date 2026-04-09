#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWindow.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor inspector window.
///        Displays the properties of the selected object.
struct InspectorWindow : Handle<struct InspectorWindowObj>
{
    InspectorWindow() = default;
    InspectorWindow(const EditorWindowObj* obj) { mObj = (InspectorWindowObj*)obj; }

    static EditorWindow create(const EditorWindowInfo& windowInfo);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
};

} // namespace LD