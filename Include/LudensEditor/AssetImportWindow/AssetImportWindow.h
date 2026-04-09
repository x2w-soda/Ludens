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

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);

    void set_type(AssetType type);
    void set_source_path(const std::string& srcPath);
};

} // namespace LD