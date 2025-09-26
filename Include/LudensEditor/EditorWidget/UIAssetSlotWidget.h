#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorCallback.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct UIAssetSlotWidgetInfo
{
    EditorTheme theme;                                                  /// editor theme handle
    UIWidget parent;                                                    /// parent widget of the asset slot
    AssetType type;                                                     /// allowed asset type for this slot
    AUID assetID;                                                       /// initial asset ID
    const char* assetName;                                              /// initial asset name
    void (*requestAssetFn)(AssetType type, AUID currentID, void* user); /// action to select a new asset for this slot
    void* user;                                                         /// used in callbacks
};

/// @brief A slot for an asset.
struct UIAssetSlotWidget : Handle<struct UIAssetSlotWidgetObj>
{
    /// @brief Create an asset slot widget.
    static UIAssetSlotWidget create(const UIAssetSlotWidgetInfo& info);

    /// @brief Destroy asset slot widget.
    static void destroy(UIAssetSlotWidget widget);

    /// @brief Set the asset to display in this slot.
    void set_asset(AUID assetID, const char* assetName);

    /// @brief Get accepted asset type.
    AssetType get_type();

    /// @brief Get current asset ID.
    AUID get_id();

    /// @brief Show widget subtree.
    void show();

    /// @brief Hide widget subtree.
    void hide();
};

} // namespace LD