#pragma once

#include "../SceneObj.h"

namespace LD {


struct AudioSourceMeta
{
    static void init(ComponentBase** dstData);
    static bool load(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, String& err);
    static bool load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, String& err);
    static bool clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err);
    static bool unload(SceneObj* scene, ComponentBase** source, String& err);
    static bool cleanup(SceneObj* scene, ComponentBase** source, String& err);
    static AssetID get_asset(SceneObj* scene, ComponentBase** source, uint32_t assetSlotIndex);
    static bool set_asset(SceneObj* scene, ComponentBase** source, uint32_t assetSlotIndex, AssetID id);
    static AssetType get_asset_type(SceneObj* scene, uint32_t assetSlotIndex);

    static TypeMeta sTypeMeta;
};

} // namespace LD