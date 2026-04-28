#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

enum AssetSelectWindowMode : EditorWindowMode
{
    ASSET_SELECT_WINDOW_MODE_DEFAULT,
    ASSET_SELECT_WINDOW_MODE_NEW_SCRIPT,
};

/// @brief Window to select assets in project by URI
struct AssetSelectWindow : Handle<struct AssetSelectWindowObj>
{
    AssetSelectWindow() = default;
    AssetSelectWindow(const EditorWindowObj* obj) { mObj = (AssetSelectWindowObj*)obj; }

    void set_filter(AssetType type);
    void set_component(SUID comp);
    void set_component_asset_slot_index(uint32_t index);

    static EditorWindow create(const EditorWindowInfo& windowI);
    static void destroy(EditorWindow window);
    static void update(EditorWindowObj* obj, const EditorUpdateTick& tick);
    static void mode_hint(EditorWindowObj* obj, EditorWindowMode modeHint);
};

} // namespace LD
