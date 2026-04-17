#pragma once

#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {
namespace EditorContextUtil {

inline void set_component_asset(EditorContext ctx, SUID compSUID, AssetID assetID, uint32_t assetSlotIndex)
{
    auto* event = (EditorActionSetComponentAssetEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET);
    event->compSUID = compSUID;
    event->assetID = assetID;
    event->assetSlotIndex = assetSlotIndex;
}

inline void set_component_script(EditorContext ctx, SUID compSUID, AssetID assetID)
{
    auto* event = (EditorActionSetComponentScriptEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_SCRIPT);
    event->compSUID = compSUID;
    event->assetID = assetID;
}

inline void request_component_asset(EditorContext ctx, SUID compSUID, AssetID assetID, AssetType assetType, uint32_t assetSlotIndex)
{
    auto* event = (EditorRequestComponentAssetEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET);
    event->compSUID = compSUID;
    event->oldAssetID = assetID;
    event->requestType = assetType;
    event->assetSlotIndex = assetSlotIndex;
}

inline void request_component_script(EditorContext ctx, SUID compSUID)
{
    auto* event = (EditorRequestComponentScriptEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_SCRIPT);
    event->compSUID = compSUID;
}

} // namespace EditorContextUtil
} // namespace LD