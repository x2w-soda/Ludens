#include <Ludens/Memory/Memory.h>
#include <LudensBuilder/AssetBuilder/AssetBuilder.h>

#include "AssetBuilderMeta.h"

namespace LD {

bool AssetImportInfo::set_src_files(size_t pathCount, const FS::Path* paths)
{
    if (sImportMeta[(int)type].setSrcFiles)
        return sImportMeta[(int)type].setSrcFiles(this, pathCount, paths);

    return false;
}

/// @brief Asset builder implementation.
struct AssetBuilderObj
{
};

AssetBuilder AssetBuilder::create()
{
    auto* obj = heap_new<AssetBuilderObj>(MEMORY_USAGE_ASSET);

    return AssetBuilder(obj);
}

void AssetBuilder::destroy(AssetBuilder builder)
{
    auto* obj = builder.unwrap();

    heap_delete<AssetBuilderObj>(obj);
}

AssetCreateInfo* AssetBuilder::allocate_create_info(AssetCreateType type)
{
    return sCreateMeta[(int)type].allocInfo();
}

void AssetBuilder::free_create_info(AssetCreateInfo* info)
{
    sCreateMeta[(int)info->type].freeInfo(info);
}

bool AssetBuilder::create_asset(AssetCreateInfo* info, std::string& err)
{
    if (!info)
    {
        err = "missing AssetCreateInfo";
        return false;
    }

    return sCreateMeta[(int)info->type].createFn(info, err);
}

bool AssetBuilder::prepare_import(const AssetCreateInfo* srcInfo, AssetImportInfo* dstInfo, std::string& err)
{
    return sCreateMeta[(int)srcInfo->type].prepareImportFn(srcInfo, dstInfo, err);
}

} // namespace LD