#include <Ludens/Asset/AssetType/Texture2DAssetObj.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetSource/TextureAssetSource.h>
#include <LudensBuilder/AssetBuilder/AssetState/Texture2DAssetState.h>

#include "../AssetImportJob.h"

namespace LD {

void texture_2d_asset_import(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetImportJob*)user;
    auto* obj = (Texture2DAssetObj*)job.asset.unwrap();
    const auto& info = *(const Texture2DAssetImportInfo*)job.info;

    obj->samplerHint = info.samplerHint;
    obj->serialData.clear();

    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_TEXTURE_2D);

    Texture2DAssetObj::serialize_sampler_info(serial, obj->samplerHint);

    uint64_t fileSize;
    if (!FS::get_file_size(info.srcFile, fileSize, job.status.str))
    {
        job.status.type = ASSET_IMPORT_ERROR_SRC_FILE;
        return;
    }

    obj->fileView.size = fileSize;

    size_t fileDataOffset = serial.write_chunk_begin("FILE");
    byte* fileData = serial.advance(fileSize);

    if (!job.read_src_file(info.srcFile, MutView(fileData, fileSize)))
        return;

    serial.write_chunk_end();

    View serialView = serial.view();

    // only retrieve address after serializer has completed all writes
    obj->fileView.data = serialView.data + fileDataOffset;

    obj->bitmap = Bitmap::create_from_file_data(obj->fileView);
    if (!job.require(obj->bitmap, "failed to create Bitmap"))
        return;

    (void)job.write_binary_dst_file(serialView);
}

bool texture_2d_asset_import_info_set_src_files(AssetImportInfo* base, size_t count, const FS::Path* srcFilePaths)
{
    if (count != 1)
        return false;

    auto* info = (Texture2DAssetImportInfo*)base;

    info->srcFile = srcFilePaths[0];
    return true;
}

} // namespace LD