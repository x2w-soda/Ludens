#pragma once

#include <LudensBuilder/AssetBuilder/AssetState/LuaScriptAssetState.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {
namespace EditorContextUtil {

inline Asset create_lua_script_asset(EditorContext ctx, const std::string& requestPath, std::string& err)
{
    EditorContextAssetInterface assetI = ctx.asset_interface();

    auto* info = (LuaScriptAssetCreateInfo*)assetI.alloc_asset_create_info(ASSET_CREATE_TYPE_LUA_SCRIPT);
    info->domain = LUA_SCRIPT_DOMAIN_COMPONENT;

    return assetI.create_asset(info, requestPath, err);
}

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

inline void set_component_props(EditorContext ctx, SUID compSUID, const Vector<PropertyDelta>& delta)
{
    if (!compSUID || delta.empty())
        return;

    auto* event = (EditorActionSetComponentPropsEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_PROPS);
    event->compSUID = compSUID;
    event->delta = delta;
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