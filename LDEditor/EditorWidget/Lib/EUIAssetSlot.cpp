#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/EUIAssetSlot.h>

#include "EUI.h"

namespace LD {

bool eui_asset_slot(AssetID assetID, AssetType assetType)
{
    AssetManager AM = AssetManager::get();
    Asset asset = AM.get_asset(assetID, assetType);

    const char* assetName = "N/A";
    if (asset)
        assetName = asset.get_name();

    Vec2 mousePos;
    MouseValue mouseVal;
    bool newAssetRequest = false;
    float childGap = 6.0f;
    EditorTheme theme = eui_get_theme();

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap);
    layoutI.sizeX = UISize::grow();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    layoutI = theme.make_text_label_layout();
    ui_push_text(nullptr, get_asset_type_cstr(assetType));
    ui_top_layout(layoutI);
    ui_pop();

    ui_push_text(nullptr, assetName);
    ui_top_layout(layoutI);
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
        newAssetRequest = true;
    ui_pop();

    ui_pop();

    return newAssetRequest;
}

} // namespace LD
