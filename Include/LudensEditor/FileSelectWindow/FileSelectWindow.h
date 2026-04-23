#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor selection window. Generic dialog for all kinds of selection tasks.
struct FileSelectWindow : Handle<struct FileSelectWindowObj>
{
    FileSelectWindow() = default;
    FileSelectWindow(const EditorWindowObj* obj) { mObj = (FileSelectWindowObj*)obj; }

    void show(const FS::Path& directoryPath, const char* extensionFilter);
    bool has_selected(FS::Path& path);

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
};

} // namespace LD