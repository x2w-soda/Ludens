#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

bool eui_asset_slot(EditorTheme theme, AssetType type, AssetID assetID, const char* assetName);

} // namespace LD