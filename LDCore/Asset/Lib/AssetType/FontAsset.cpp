#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/FontAssetObj.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetLoadJob.h"
#include "../AssetMeta.h"

namespace LD {

bool FontAssetObj::load_from_binary(AssetLoadJob& job, const FS::Path& filePath)
{
    Vector<byte> tmp;
    if (!job.read_file_to_vector(filePath, tmp))
        return false;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return false;

    if (type != ASSET_TYPE_FONT)
        return false;

    serial.read_f32(fontSize);

    uint32_t fontDataSize;
    serial.read_u32(fontDataSize);
    const byte* fontData = serial.view_now();

    font = Font::create_from_memory(fontData, (size_t)fontDataSize);
    if (!job.require(font, "failed to create Font"))
        return false;

    fontAtlas = FontAtlas::create_bitmap(font, fontSize);
    if (!job.require(fontAtlas, "failed to create FontAtlas"))
        return false;

    return true;
}

void FontAssetObj::create(AssetObj* base)
{
    new (base) FontAssetObj();
}

void FontAssetObj::destroy(AssetObj* base)
{
    ((FontAssetObj*)base)->~FontAssetObj();
}

void FontAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    auto* obj = (FontAssetObj*)job.assetHandle.unwrap();

    FS::Path filePath = job.assetDirPath / LD_ASSET_DEFAULT_BINARY_FILE_NAME;
    if (FS::exists(filePath))
        obj->load_from_binary(job, filePath);
}

void FontAssetObj::unload(AssetObj* base)
{
    auto* obj = (FontAssetObj*)base;

    if (obj->fontAtlas)
    {
        FontAtlas::destroy(obj->fontAtlas);
        obj->fontAtlas = {};
    }

    if (obj->font)
    {
        Font::destroy(obj->font);
        obj->font = {};
    }
}

Font FontAsset::get_font()
{
    auto* obj = (FontAssetObj*)mObj;

    return obj->font;
}

FontAtlas FontAsset::get_font_atlas()
{
    auto* obj = (FontAssetObj*)mObj;

    return obj->fontAtlas;
}

} // namespace LD