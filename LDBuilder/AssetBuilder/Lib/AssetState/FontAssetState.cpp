#include <Ludens/Asset/AssetType/FontAssetObj.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetSource/FontAssetSource.h>
#include <LudensBuilder/AssetBuilder/AssetState/FontAssetState.h>

#include "../AssetImportJob.h"

namespace LD {

void font_asset_import(void* user)
{
    auto& job = *(AssetImportJob*)user;
    auto* obj = (FontAssetObj*)job.asset.unwrap();
    const auto& info = *(FontAssetImportInfo*)job.info;

    std::vector<byte> tmpSourceData;
    const byte* sourceData = nullptr;
    size_t sourceDataSize = 0;

    if (info.srcView)
    {
        sourceData = (const byte*)info.srcView.data;
        sourceDataSize = info.srcView.size;
    }
    else
    {
        if (!job.read_src_file_to_vector(info.srcFile, tmpSourceData))
            return;
        sourceData = tmpSourceData.data();
        sourceDataSize = tmpSourceData.size();
    }

    obj->font = Font::create_from_memory(sourceData, sourceDataSize);
    obj->fontSize = info.fontSize;
    obj->fontAtlas = FontAtlas::create_bitmap(obj->font, obj->fontSize);

    if (!job.require(obj->fontAtlas, "failed to create bitmap FontAtlas"))
        return;

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_FONT);

    serializer.write_f32(obj->fontSize);
    serializer.write_u32(sourceDataSize);
    serializer.write(sourceData, sourceDataSize);

    (void)job.write_binary_dst_file(serializer.view());
}

bool font_asset_import_info_set_src_files(AssetImportInfo* base, size_t count, const FS::Path* srcFilePaths)
{
    if (count != 1)
        return false;

    auto* info = (FontAssetImportInfo*)base;

    info->srcFile = srcFilePaths[0];
    return true;
}

} // namespace LD