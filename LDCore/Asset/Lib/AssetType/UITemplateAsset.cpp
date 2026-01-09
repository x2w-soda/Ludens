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

    std::string err; // TODO:
    Serializer serializer(fileSize);
    if (!FS::read_file(job.loadPath, MutView((char*)serializer.data(), fileSize), err))
        return;

    View fileView((const char*)serializer.data(), serializer.size());

    bool ok = UITemplateSchema::load_ui_template_from_source(obj->tmpl, fileView, err);
    LD_ASSERT(ok); // TODO: asset load failure
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