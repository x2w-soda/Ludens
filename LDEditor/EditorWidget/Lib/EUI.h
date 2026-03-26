#pragma once

#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

EditorTheme eui_get_theme();
EditorContext eui_get_context();

inline void eui_ctx_request_asset(SUID compID, AssetID assetID, AssetType assetType)
{
    // NOTE: will have to refactor, this assumes single asset slot for all component types
    auto* event = (EditorRequestComponentAssetEvent*)eui_get_context().enqueue_event(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET);
    event->component = compID;
    event->oldAssetID = assetID;
    event->requestType = assetType;
}

} // namespace LD