#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Memory/Allocator.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>

#include <cctype>
#include <format>

#include "AssetBuilderMeta.h"
#include "AssetImportJob.h"

#define LD_ASSERT_IMPORT_SCOPE LD_ASSERT(mObj->isBatchScope)
#define LD_ASSERT_NOT_IMPORT_SCOPE LD_ASSERT(!mObj->isBatchScope)

namespace LD {

struct AssetImporterObj
{
    SUIDRegistry suidRegistry = {};          // if valid, used to validate and generate SUIDs
    PoolAllocator importJobPA = {};          // pool allocator for import jobs
    PoolAllocator importInfoPA = {};         // pool allocator for import infos
    Vector<AssetImportJob*> importJobs = {}; // all jobs in a batch
    FS::Path projectRootDir = {};
    bool isBatchScope = false;

    Asset reserve_asset(AssetType type);
    void resolve_asset(AssetImportJob* job);
    AssetImportInfo* allocate_import_info(AssetType type);
    void free_import_info(AssetImportInfo* asset);
    AssetImportJob* allocate_import_job(AssetType type);
    void free_import_job(AssetImportJob* job);
    void clear_import_jobs();
    AssetImportStatus check_base_info(const AssetImportInfo* info);
};

Asset AssetImporterObj::reserve_asset(AssetType type)
{
    AssetManager AM = AssetManager::get();
    LD_ASSERT(AM);

    return AM.alloc_reserved_asset(suidRegistry, type);
}

// NOTE: This is the last step that decides whether the import transaction succeeds.
//       The import task succeeds iff the AssetRegistry (referenced by AssetManager)
//       registers the imported asset as a new entry.
void AssetImporterObj::resolve_asset(AssetImportJob* job)
{
    LD_ASSERT(job->status);

    AssetManager AM = AssetManager::get();
    AssetEntry entry = AM.resolve_asset(suidRegistry, job->asset, job->info->dstPath);

    if (!entry)
    {
        job->status.str = "failed to register asset in AssetRegistry";
        job->status.type = ASSET_IMPORT_ERROR;
    }
    else
    {
        for (const auto& it : job->files)
            entry.set_file_path(it.first, it.second.string());
    }
}

AssetImportInfo* AssetImporterObj::allocate_import_info(AssetType type)
{
    return sImportMeta[(int)type].allocInfo();
}

void AssetImporterObj::free_import_info(AssetImportInfo* info)
{
    sImportMeta[(int)info->type].freeInfo(info);
}

AssetImportJob* AssetImporterObj::allocate_import_job(AssetType type)
{
    AssetImportJob* job = (AssetImportJob*)importJobPA.allocate();
    new (job) AssetImportJob();

    return job;
}

void AssetImporterObj::free_import_job(AssetImportJob* job)
{
    if (job->info) // consume
        free_import_info(job->info);

    job->~AssetImportJob();
    importJobPA.free(job);
}

void AssetImporterObj::clear_import_jobs()
{
    for (AssetImportJob* job : importJobs)
        free_import_job(job);

    importJobs.clear();
}

AssetImportStatus AssetImporterObj::check_base_info(const AssetImportInfo* info)
{
    AssetImportStatus status{};
    status.type = ASSET_IMPORT_SUCCESS;

    AssetManager AM = AssetManager::get();
    AssetRegistry registry = AM.get_asset_registry();

    if (info->dstPath.empty())
    {
        status.type = ASSET_IMPORT_ERROR_DST_PATH;
        status.str = "requested URI path is empty";
    }
    else if (registry && registry.get_entry_by_path(info->dstPath))
    {
        status.type = ASSET_IMPORT_ERROR_DST_PATH;
        status.str = "URI path already registered in project";
    }

    return status;
}

//
// Public API
//

AssetImporter AssetImporter::create()
{
    auto* obj = heap_new<AssetImporterObj>(MEMORY_USAGE_ASSET);

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(AssetImportJob);
    paI.isMultiPage = true;
    paI.pageSize = 16;
    paI.usage = MEMORY_USAGE_ASSET;
    obj->importJobPA = PoolAllocator::create(paI);

    return AssetImporter(obj);
}

void AssetImporter::destroy(AssetImporter importer)
{
    auto* obj = importer.unwrap();

    PoolAllocator::destroy(obj->importJobPA);

    heap_delete<AssetImporterObj>(obj);
}

void AssetImporter::set_resolve_params(SUIDRegistry idReg, FS::Path projectRootDir)
{
    mObj->suidRegistry = idReg;
    mObj->projectRootDir = projectRootDir;
}

AssetImportInfo* AssetImporter::allocate_import_info(AssetType type)
{
    return mObj->allocate_import_info(type);
}

void AssetImporter::free_import_info(AssetImportInfo* asset)
{
    mObj->free_import_info(asset);
}

void AssetImporter::import_batch_begin()
{
    LD_ASSERT_NOT_IMPORT_SCOPE;

    mObj->isBatchScope = true;

    mObj->clear_import_jobs();
}

void AssetImporter::import_batch_end()
{
    LD_ASSERT_IMPORT_SCOPE;

    mObj->isBatchScope = false;
}

void AssetImporter::import_batch_asset(AssetImportInfo* info)
{
    LD_ASSERT_IMPORT_SCOPE;
    LD_ASSERT(info && info->type != ASSET_TYPE_ENUM_COUNT);

    AssetImportJob* job = mObj->allocate_import_job(info->type);
    job->status = mObj->check_base_info(info);
    job->asset = mObj->reserve_asset(info->type);
    job->assetDir = FS::absolute(mObj->projectRootDir / "storage" / job->asset.get_id().to_string());
    job->info = info;

    mObj->importJobs.push_back(job);

    if (!job->status)
    {
        job->hasCompleted.store(true);
        return; // skip submission
    }

    job->submit();
}

bool AssetImporter::import_asset_async(AssetImportResult& outResult)
{
    LD_ASSERT_NOT_IMPORT_SCOPE;

    AssetImportJob* completedJob = nullptr;

    for (AssetImportJob* job : mObj->importJobs)
    {
        if (job->has_completed())
        {
            completedJob = job;

            if (job->status)
                mObj->resolve_asset(job);

            outResult.status = job->status;
            outResult.dstAsset = job->asset;
            break;
        }
    }

    if (completedJob)
    {
        std::erase(mObj->importJobs, completedJob);
        mObj->free_import_job(completedJob);
        return true;
    }

    return false;
}

AssetImportResult AssetImporter::import_asset_synchronous(AssetImportInfo* info)
{
    LD_ASSERT_NOT_IMPORT_SCOPE;
    LD_ASSERT(info && info->type != ASSET_TYPE_ENUM_COUNT);

    AssetImportJob* job = mObj->allocate_import_job(info->type);
    job->status = mObj->check_base_info(info);
    job->asset = mObj->reserve_asset(info->type);
    job->assetDir = FS::absolute(mObj->projectRootDir / "storage" / job->asset.get_id().to_string());
    job->info = info;

    AssetImportResult result{};
    result.dstAsset = job->asset;
    result.status = job->status;

    if (job->status)
        job->execute_synchronous();
    else
    {
        job->hasCompleted.store(true);
        result.dstAsset = {};
    }

    LD_ASSERT(job->has_completed());

    if (job->status)
        mObj->resolve_asset(job);
    else
    {
        result.dstAsset = {};
    }

    mObj->free_import_job(job);

    return result;
}

bool AssetImporter::get_asset_type_from_path(const FS::Path& path, AssetType& outType)
{
    outType = ASSET_TYPE_ENUM_COUNT;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](int c) { return std::tolower(c); });

    // TODO: generalize text search utilities
    if (ext == ".mp3" || ext == ".wav")
        outType = ASSET_TYPE_AUDIO_CLIP;
    else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
        outType = ASSET_TYPE_TEXTURE_2D;
    else if (ext == ".ttf")
        outType = ASSET_TYPE_FONT;
    else if (ext == ".lua")
        outType = ASSET_TYPE_LUA_SCRIPT;

    return outType != ASSET_TYPE_ENUM_COUNT;
}

} // namespace LD