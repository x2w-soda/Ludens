#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor selection window. Generic dialog for all kinds of selection tasks.
struct SelectionWindow : Handle<struct SelectionWindowObj>
{
    SelectionWindow() = default;
    SelectionWindow(const EditorWindowObj* obj) { mObj = (SelectionWindowObj*)obj; }

    /// @brief Create editor selection window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy editor selection window.
    static void destroy(EditorWindow window);

    void show(const FS::Path& directoryPath, const char* extensionFilter);
    bool has_selected(FS::Path& path);
    bool has_canceled();
};

} // namespace LD