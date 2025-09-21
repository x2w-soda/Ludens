#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct UIAssetSlotWidgetInfo
{
    EditorTheme theme; /// editor theme handle
    UIWidget parent;   /// parent widget of the asset slot
    AssetType type;    /// allowed asset type for this slot
    AUID* asset;       /// address of some asset handle, must outlive the widget
};

/// @brief A slot for an asset.
struct UIAssetSlotWidget : Handle<struct UIAssetSlotWidgetObj>
{
    /// @brief Create an asset slot widget
    static UIAssetSlotWidget create(const UIAssetSlotWidgetInfo& info);

    /// @brief Destroy asset slot widget
    static void destroy(UIAssetSlotWidget widget);

    /// @brief Set the asset handle address to edit.
    void set(AUID* asset);

    /// @brief Set the name of asset to display
    void set_asset_name(const char* assetName);

    /// @brief Show widget subtree.
    void show();

    /// @brief Hide widget subtree.
    void hide();

    // TODO: experimental
    void select();
};

} // namespace LD