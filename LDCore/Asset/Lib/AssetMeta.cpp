#include <Ludens/Header/Version.h>

#include "AssetMeta.h"
#include "AssetType/AudioClipAssetObj.h"
#include "AssetType/BlobAssetObj.h"
#include "AssetType/FontAssetObj.h"
#include "AssetType/LuaScriptAssetObj.h"
#include "AssetType/MeshAssetObj.h"
#include "AssetType/Texture2DAssetObj.h"
#include "AssetType/TextureCubeAssetObj.h"
#include "AssetType/UITemplateAssetObj.h"

namespace LD {

// clang-format off
AssetMeta sAssetMeta[] = {
    {ASSET_TYPE_BLOB,         "Blob",        sizeof(BlobAssetObj),        &BlobAssetObj::load,         &BlobAssetObj::unload, },
    {ASSET_TYPE_FONT,         "Font",        sizeof(FontAssetObj),        &FontAssetObj::load,         &FontAssetObj::unload, },
    {ASSET_TYPE_MESH,         "Mesh",        sizeof(MeshAssetObj),        &MeshAssetObj::load,         &MeshAssetObj::unload, },
    {ASSET_TYPE_UI_TEMPLATE,  "UITemplate",  sizeof(UITemplateAssetObj),  &UITemplateAssetObj::load,   &UITemplateAssetObj::unload, },
    {ASSET_TYPE_AUDIO_CLIP,   "AudioClip",   sizeof(AudioClipAssetObj),   &AudioClipAssetObj::load,    &AudioClipAssetObj::unload, },
    {ASSET_TYPE_TEXTURE_2D,   "Texture2D",   sizeof(Texture2DAssetObj),   &Texture2DAssetObj::load,    &Texture2DAssetObj::unload, },
    {ASSET_TYPE_TEXTURE_CUBE, "TextureCube", sizeof(TextureCubeAssetObj), &TextureCubeAssetObj::load,  &TextureCubeAssetObj::unload, },
    {ASSET_TYPE_LUA_SCRIPT,   "LuaScript",   sizeof(LuaScriptAssetObj),   &LuaScriptAssetObj::load,    &LuaScriptAssetObj::unload, },
};
// clang-format on

static_assert(sizeof(sAssetMeta) / sizeof(*sAssetMeta) == ASSET_TYPE_ENUM_COUNT);
static_assert(LD::IsTrivial<AssetObj>);
static_assert(LD::IsTrivial<BlobAssetObj>);
static_assert(LD::IsTrivial<FontAssetObj>);
static_assert(LD::IsTrivial<MeshAssetObj>);
static_assert(LD::IsTrivial<UITemplateAssetObj>);
static_assert(LD::IsTrivial<AudioClipAssetObj>);
static_assert(LD::IsTrivial<Texture2DAssetObj>);
static_assert(LD::IsTrivial<TextureCubeAssetObj>);
static_assert(LD::IsTrivial<LuaScriptAssetObj>);

void asset_unload(AssetObj* base)
{
    LD_ASSERT(base);

    if (!sAssetMeta[base->type].unload)
        return;

    sAssetMeta[base->type].unload(base);
}

const char* get_asset_type_name_cstr(AssetType type)
{
    return sAssetMeta[(int)type].typeName;
}

void asset_header_write(Serializer& serial, AssetType type)
{
    serial.write((const byte*)LD_ASSET_MAGIC, 4);
    serial.write_u16(LD_VERSION_MAJOR);
    serial.write_u16(LD_VERSION_MINOR);
    serial.write_u16(LD_VERSION_PATCH);

    const char* typeName = get_asset_type_name_cstr(type);
    size_t len = strlen(typeName);

    serial.write_u16((uint16_t)len);
    serial.write((const byte*)typeName, len);
}

bool asset_header_read(Deserializer& serial, uint16_t& outMajor, uint16_t& outMinor, uint16_t& outPatch, AssetType& outType)
{
    if (serial.size() < 12)
        return false;

    char ldaMagic[4];
    serial.read((byte*)ldaMagic, 4);

    if (strncmp(ldaMagic, LD_ASSET_MAGIC, 4))
        return false;

    serial.read_u16(outMajor);
    serial.read_u16(outMinor);
    serial.read_u16(outPatch);

    uint16_t len;
    serial.read_u16(len);

    std::string typeName;
    typeName.resize(len);
    serial.read((byte*)typeName.data(), typeName.size());

    // TODO: this can be hashed for O(1) lookup but we barely have any asset type variety right now.
    for (int type = 0; type < (int)ASSET_TYPE_ENUM_COUNT; type++)
    {
        const char* match = get_asset_type_name_cstr((AssetType)type);

        if (typeName == match)
        {
            outType = (AssetType)type;
            return true;
        }
    }

    // unrecognized asset type
    return false;
}

bool asset_header_read(Deserializer& serial, AssetType expectedType, Diagnostics& diag)
{
    DiagnosticScope scope(diag, "asset_header_read");

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
    {
        diag.mark_error("failed to recognize binary asset header");
        return false;
    }

    if (type != expectedType)
    {
        diag.mark_error(std::format("expected asset type {}, found {}", get_asset_type_cstr(expectedType), get_asset_type_cstr(type)));
        return false;
    }

    if (major != LD_VERSION_MAJOR || minor != LD_VERSION_MINOR || patch != LD_VERSION_PATCH)
    {
        diag.mark_error(std::format("expected asset version {}.{}.{}, found {}.{}.{}",
                                    LD_VERSION_MAJOR, LD_VERSION_MINOR, LD_VERSION_PATCH, major, minor, patch));
        return false;
    }

    return true;
}

} // namespace LD