#include <Ludens/Asset/AssetType/UITemplateAsset.h>
#include <Ludens/Asset/Template/UISchema.h>
#include <Ludens/Profiler/Profiler.h>

#include <format>

#include "../AssetMeta.h"
#include "UITemplateAssetObj.h"

namespace LD {

void UITemplateAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    auto* obj = (UITemplateAssetObj*)job.assetHandle.unwrap();

    std::string err;
    Vector<byte> file;
    FS::read_file_to_vector(job.loadPath, file, err);
    View fileView((const char*)file.data(), file.size());

    obj->tmpl = UITemplate::create();

    UISchema::Error schemaErr;
    if (!UISchema::load_ui_template_from_source(obj->tmpl, fileView, schemaErr))
    {
        job.diagnostics.mark_error(std::format("failed to load UITemplate: {}", schemaErr.str));
        return;
    }

    FS::Path sourcePath = job.rootPath / FS::Path(job.assetEntry.get_path("source"));

    obj->luaSource = FS::read_file_to_cstr(sourcePath, err);
    if (!obj->luaSource)
    {
        job.diagnostics.mark_error(std::format("failed to load UITemplate script: {}", sourcePath.string()));
        return;
    }
}

void UITemplateAssetObj::unload(AssetObj* base)
{
    auto* obj = (UITemplateAssetObj*)base;

    if (obj->luaSource)
    {
        heap_free(obj->luaSource);
        obj->luaSource = nullptr;
    }

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

const char* UITemplateAsset::get_lua_source()
{
    auto* obj = (UITemplateAssetObj*)mObj;

    return obj->luaSource;
}

} // namespace LD