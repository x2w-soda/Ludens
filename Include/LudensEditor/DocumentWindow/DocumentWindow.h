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

    /// @brief Create a document window
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy document window.
    static void destroy(EditorWindow window);
};

} // namespace LD