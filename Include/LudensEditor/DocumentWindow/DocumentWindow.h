#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/UI/UIWorkspace.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Displays a Document.
struct DocumentWindow : Handle<struct DocumentWindowObj>
{
    DocumentWindow() = default;
    DocumentWindow(const EditorWindowObj* obj) { mObj = (DocumentWindowObj*)obj; }

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void pre_update(EditorWindowObj* obj, const EditorUpdateTick& tick);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
};

} // namespace LD