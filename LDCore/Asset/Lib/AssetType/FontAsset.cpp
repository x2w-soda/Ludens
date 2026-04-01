#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/FontAssetObj.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetMeta.h"

namespace LD {

void FontAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    auto* obj = (FontAssetObj*)job.assetHandle.unwrap();

    std::string err; // TODO:
    std::vector<byte> tmp;
    if (!FS::read_file_to_vector(job.loadPath, tmp, err))
        return;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_FONT)
        return;

    serial.read_f32(obj->fontSize);

    uint32_t fontDataSize;
    serial.read_u32(fontDataSize);
    const byte* fontData = serial.view_now();

    obj->font = Font::create_from_memory(fontData, (size_t)fontDataSize);
    obj->fontAtlas = FontAtlas::create_bitmap(obj->font, obj->fontSize);
}

void FontAssetObj::unload(AssetObj* base)
{
    auto* obj = (FontAssetObj*)base;
    FontAtlas::destroy(obj->fontAtlas);
    obj->fontAtlas = {};

    Font::destroy(obj->font);
    obj->font = {};
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