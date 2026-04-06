#include <Ludens/Header/Assert.h>
#include <LudensBuilder/AssetBuilder/AssetSource/AssetSources.h>
#include <LudensBuilder/AssetBuilder/AssetState/AssetStates.h>

#include "AssetBuilderMeta.h"

#define MEMORY_USAGE MEMORY_USAGE_MISC

namespace LD {

static AssetImportInfo* blob_asset_import_info_new() { return heap_new<BlobAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* font_asset_import_info_new() { return heap_new<FontAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* mesh_asset_import_info_new() { return heap_new<MeshAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* ui_template_asset_import_info_new() { return heap_new<UITemplateAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* audio_clip_import_info_new() { return heap_new<AudioClipAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* texture_2d_import_info_new() { return heap_new<Texture2DAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* texture_cube_import_info_new() { return heap_new<TextureCubeAssetImportInfo>(MEMORY_USAGE); }
static AssetImportInfo* lua_script_import_info_new() { return heap_new<LuaScriptAssetImportInfo>(MEMORY_USAGE); }
static void blob_asset_import_info_delete(AssetImportInfo* info) { heap_delete<BlobAssetImportInfo>((BlobAssetImportInfo*)info); }
static void font_asset_import_info_delete(AssetImportInfo* info) { heap_delete<FontAssetImportInfo>((FontAssetImportInfo*)info); }
static void mesh_asset_import_info_delete(AssetImportInfo* info) { heap_delete<MeshAssetImportInfo>((MeshAssetImportInfo*)info); }
static void ui_template_asset_import_info_delete(AssetImportInfo* info) { heap_delete<UITemplateAssetImportInfo>((UITemplateAssetImportInfo*)info); }
static void audio_clip_import_info_delete(AssetImportInfo* info) { heap_delete<AudioClipAssetImportInfo>((AudioClipAssetImportInfo*)info); }
static void texture_2d_import_info_delete(AssetImportInfo* info) { heap_delete<Texture2DAssetImportInfo>((Texture2DAssetImportInfo*)info); }
static void texture_cube_import_info_delete(AssetImportInfo* info) { heap_delete<TextureCubeAssetImportInfo>((TextureCubeAssetImportInfo*)info); }
static void lua_script_import_info_delete(AssetImportInfo* info) { heap_delete<LuaScriptAssetImportInfo>((LuaScriptAssetImportInfo*)info); }

// clang-format off
AssetBuilderMeta sBuilderMeta[] = {
    {&blob_asset_import_info_new,        &blob_asset_import_info_delete,        &blob_asset_import},
    {&font_asset_import_info_new,        &font_asset_import_info_delete,        &font_asset_import},
    {&mesh_asset_import_info_new,        &mesh_asset_import_info_delete,        &mesh_asset_import},
    {&ui_template_asset_import_info_new, &ui_template_asset_import_info_delete, &ui_template_asset_import},
    {&audio_clip_import_info_new,        &audio_clip_import_info_delete,        &audio_clip_asset_import},
    {&texture_2d_import_info_new,        &texture_2d_import_info_delete,        &texture_2d_asset_import},
    {&texture_cube_import_info_new,      &texture_cube_import_info_delete,      &texture_cube_asset_import},
    {&lua_script_import_info_new,        &lua_script_import_info_delete,        &lua_script_asset_import},
};
// clang-format on

static_assert(sizeof(sBuilderMeta) / sizeof(*sBuilderMeta) == (int)ASSET_TYPE_ENUM_COUNT);

} // namespace LD