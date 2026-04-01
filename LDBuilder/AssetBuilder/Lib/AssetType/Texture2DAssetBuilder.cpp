#include <Ludens/Asset/AssetType/Texture2DAssetObj.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfoStorage.h>
#include <LudensBuilder/AssetBuilder/AssetType/Texture2DAssetBuilder.h>

#include "../AssetImportJob.h"

namespace LD {

void texture_2d_asset_copy_import_info(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo)
{
    dstInfo.as.texture2D = *(Texture2DAssetImportInfo*)srcInfo;
}

void texture_2d_asset_import(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetImportJob*)user;
    auto* obj = (Texture2DAssetObj*)job.asset.unwrap();
    const Texture2DAssetImportInfo& info = job.info.as.texture2D;

    obj->id = 0;
    obj->samplerHint = info.samplerHint;
    obj->serialData = nullptr;

    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_TEXTURE_2D);

    Texture2DAssetObj::serialize_sampler_info(serial, obj->samplerHint);

    uint64_t fileSize;
    if (!FS::get_file_size(info.srcPath, fileSize, job.status.str))
    {
        job.status.type = ASSET_IMPORT_ERROR_SRC_PATH;
        return;
    }

    obj->fileSize = fileSize;

    size_t fileDataOffset = serial.write_chunk_begin("FILE");
    byte* fileData = serial.advance(fileSize);

    if (!FS::read_file(info.srcPath, MutView((char*)fileData, fileSize), job.status.str))
    {
        job.status.type = ASSET_IMPORT_ERROR_SRC_PATH;
        return;
    }

    serial.write_chunk_end();

    View serialView = serial.view();

    // only retrieve address after serializer has completed all writes
    obj->fileData = serialView.data + fileDataOffset;

    if (!FS::write_file(info.dstPath, serialView, job.status.str))
    {
        job.status.type = ASSET_IMPORT_ERROR_DST_PATH;
        return;
    }

    obj->bitmap = Bitmap::create_from_file_data(obj->fileSize, obj->fileData);
    if (!obj->bitmap)
    {
        job.status.type = ASSET_IMPORT_ERROR;
        job.status.str = "failed to create Bitmap";
    }
}

} // namespace LD