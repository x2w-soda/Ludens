#pragma once

#include <Ludens/Serial/SUID.h>

#define LD_ASSET_URI_SCHEME_AUTHORITY "ld://asset/"
#define LD_ASSET_URI_SCHEME "ld"
#define LD_ASSET_URI_AUTHORITY "asset"
#define LD_ASSET_DEFAULT_SCHEMA_FILE_NAME "asset.toml"
#define LD_ASSET_DEFAULT_SCHEMA_FILE_KEY "schema"
#define LD_ASSET_DEFAULT_BINARY_FILE_NAME "asset.lda"
#define LD_ASSET_DEFAULT_BINARY_FILE_KEY "binary"

namespace LD {

struct AssetLoadJob;

#if 0
enum AssetType
{
    ASSET_TYPE_BLOB = 0,
    ASSET_TYPE_FONT,
    ASSET_TYPE_MESH,
    ASSET_TYPE_UI_TEMPLATE,
    ASSET_TYPE_AUDIO_CLIP,
    ASSET_TYPE_TEXTURE_2D,
    ASSET_TYPE_TEXTURE_CUBE,
    ASSET_TYPE_LUA_SCRIPT,
    ASSET_TYPE_ENUM_COUNT,
};
#else
enum AssetType
{
    ASSET_TYPE_FONT = 0,
    ASSET_TYPE_AUDIO_CLIP,
    ASSET_TYPE_TEXTURE_2D,
    ASSET_TYPE_LUA_SCRIPT,
    ASSET_TYPE_ENUM_COUNT,
};
#endif

using AssetID = SUID;

} // namespace LD