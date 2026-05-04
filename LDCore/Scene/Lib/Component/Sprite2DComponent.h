#pragma once

#include "../SceneObj.h"

namespace LD {

struct Sprite2DMeta
{
    static TypeMeta sTypeMeta;

    static void init(ComponentBase** dstData);
    static bool load_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D, std::string& err);
    static bool load_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D, std::string& err);
    static bool load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, std::string& err);
    static bool clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
    static bool unload(SceneObj* scene, ComponentBase** data, std::string& err);
    static bool startup(SceneObj* scene, ComponentBase** data, std::string& err);
    static bool cleanup(SceneObj* scene, ComponentBase** data, std::string& err);
    static AssetID get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex);
    static bool set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID id);
    static AssetType get_asset_type(SceneObj* scene, uint32_t assetSlotIndex);
};

} // namespace LD