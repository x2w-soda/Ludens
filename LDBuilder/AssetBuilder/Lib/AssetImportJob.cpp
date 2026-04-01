#include <Ludens/JobSystem/JobSystem.h>
#include <LudensBuilder/AssetBuilder/AssetType/AssetBuilders.h>

#include "AssetImportJob.h"

namespace LD {

struct AssetImportMeta
{
    void (*copy_info_fn)(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo);
    void (*execute_fn)(void*);
};

// clang-format off
static AssetImportMeta sImportMeta[] = {
    {&blob_asset_copy_import_info,         &blob_asset_import},
    {&font_asset_copy_import_info,         &font_asset_import},
    {&mesh_asset_copy_import_info,         &mesh_asset_import},
    {&ui_template_asset_copy_import_info,  &ui_template_asset_import},
    {&audio_clip_asset_copy_import_info,   &audio_clip_asset_import},
    {&texture_2d_asset_copy_import_info,   &texture_2d_asset_import},
    {&texture_cube_asset_copy_import_info, &texture_cube_asset_import},
    {&lua_script_asset_copy_import_info,   &lua_script_asset_import},
};
// clang-format on

static_assert(sizeof(sImportMeta) / sizeof(*sImportMeta) == (int)ASSET_TYPE_ENUM_COUNT);

void AssetImportJob::copy_info(const AssetImportInfo* srcInfo)
{
    sImportMeta[(int)info.type].copy_info_fn(info, srcInfo);
}

void AssetImportJob::submit()
{
    status.type = ASSET_IMPORT_SUCCESS;
    status.str.clear();

    JobHeader header{};
    header.onExecute = sImportMeta[(int)info.type].execute_fn;
    header.onComplete = [](void* user) { ((AssetImportJob*)user)->hasCompleted.store(true, std::memory_order_release); };
    header.user = this;
    JobSystem::get().submit(&header, JOB_DISPATCH_STANDARD);
}

void AssetImportJob::execute_synchronous()
{
    status.type = ASSET_IMPORT_SUCCESS;
    status.str.clear();

    sImportMeta[(int)info.type].execute_fn(this);
}

} // namespace LD