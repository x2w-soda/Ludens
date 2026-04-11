#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Window to select assets in project by URI
struct AssetSelectWindow : Handle<struct AssetSelectWindowObj>
{
    AssetSelectWindow() = default;
    AssetSelectWindow(const EditorWindowObj* obj) { mObj = (AssetSelectWindowObj*)obj; }

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);

    void set_filter(AssetType type);
    void set_component(SUID comp);
};

} // namespace LD
