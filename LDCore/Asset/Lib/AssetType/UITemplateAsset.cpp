#include <Ludens/Asset/AssetType/UITemplateAsset.h>
#include <Ludens/Asset/Template/UITemplateSchema.h>
#include <Ludens/Profiler/Profiler.h>

#include "UITemplateAssetObj.h"

namespace LD {

void UITemplateAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    auto* obj = (UITemplateAssetObj*)job.assetHandle.unwrap();

    obj->tmpl = UITemplate::create();

    uint64_t fileSize = FS::get_file_size(job.loadPath);
    if (fileSize == 0)
    {
        UITemplate::destroy(obj->tmpl);
        obj->tmpl = {};
        return;
    }

    Serializer serializer(fileSize);
    FS::read_file(job.loadPath, fileSize, serializer.data());
    UITemplateSchema::load_ui_template_from_source(obj->tmpl, serializer.data(), serializer.size());
}

void UITemplateAssetObj::unload(AssetObj* base)
{
    auto* obj = (UITemplateAssetObj*)base;

    UITemplate::destroy(obj->tmpl);
    obj->tmpl = {};
}

//
// Public API
//

UIWidget UITemplateAsset::load_ui_subtree(UIWidget parent, UITemplateOnLoadCallback callback, void* user)
{
    auto* obj = (UITemplateAssetObj*)mObj;

    return obj->tmpl.load(parent, callback, user);
}

} // namespace LD