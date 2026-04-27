#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Window to import assets into project.
struct AssetImportWindow : Handle<struct AssetImportWindowObj>
{
    AssetImportWindow() = default;
    AssetImportWindow(const EditorWindowObj* obj) { mObj = (AssetImportWindowObj*)obj; }

    void set_type(AssetType type);
    void set_source_file(const FS::Path& srcFilePath);

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
};

} // namespace LD