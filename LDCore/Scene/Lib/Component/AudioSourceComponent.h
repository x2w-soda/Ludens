#pragma once

#include "../SceneObj.h"

namespace LD {


struct AudioSourceMeta
{
    static void init(ComponentBase** dstData);
    static bool load(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, std::string& err);
    static bool load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, std::string& err);
    static bool clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
    static bool unload(SceneObj* scene, ComponentBase** source, std::string& err);
    static bool cleanup(SceneObj* scene, ComponentBase** source, std::string& err);
    static AssetID get_asset(SceneObj* scene, ComponentBase** source, uint32_t assetSlotIndex);
    static bool set_asset(SceneObj* scene, ComponentBase** source, uint32_t assetSlotIndex, AssetID id);
    static AssetType get_asset_type(SceneObj* scene, uint32_t assetSlotIndex);

    static TypeMeta sTypeMeta;
};

} // namespace LD