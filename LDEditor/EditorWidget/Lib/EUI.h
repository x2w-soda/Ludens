#pragma once

#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

EditorTheme eui_get_theme();
EditorContext eui_get_context();

inline void eui_ctx_request_asset(SUID compID, AssetID assetID, AssetType assetType)
{
    // NOTE: will have to refactor, this assumes single asset slot for all component types
    EditorRequestComponentAssetEvent event(compID, assetID, assetType);

    eui_get_context().request_event(&event);
}

} // namespace LD