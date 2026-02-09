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

    /// @brief Create editor inspector window.
    static EditorWindow create(const EditorWindowInfo& windowInfo);

    /// @brief Destroy editor inspector window.
    static void destroy(EditorWindow window);

    /// @brief Check if a component is requesting a new asset.
    bool has_component_asset_request(SUID& compSUID, AssetID& currentAssetID, AssetType& assetType);
};

} // namespace LD