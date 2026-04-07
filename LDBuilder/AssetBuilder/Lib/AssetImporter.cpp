#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Memory/Allocator.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>

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
    bool isBatchScope = false;

    Asset reserve_asset(AssetType type);
    AssetImportStatus resolve_asset(Asset asset, const AssetImportInfo* info);
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

    return AM.reserve_asset(type);
}

// NOTE: This is the last step that decides whether the import transaction succeeds.
//       The import task succeeds iff the AssetRegistry (referenced by AssetManager)
//       registers the imported asset as a new entry.
AssetImportStatus AssetImporterObj::resolve_asset(Asset asset, const AssetImportInfo* info)
{
    AssetImportStatus status{};
    status.type = ASSET_IMPORT_SUCCESS;

    AssetManager AM = AssetManager::get();
    AssetEntry entry = AM.resolve_asset(suidRegistry, asset, info->dstURI);

    if (!entry)
    {
        status.str = "failed to register asset in AssetRegistry";
        status.type = ASSET_IMPORT_ERROR;
        return status;
    }

    LD_UNREACHABLE; // TODO: this should be relative to prjoect root
    entry.set_path("main", info->dstPath.string());

    return status;
}

AssetImportInfo* AssetImporterObj::allocate_import_info(AssetType type)
{
    AssetImportInfo* info = sBuilderMeta[(int)type].alloc_info();
    info->type = type;
    return info;
}

void AssetImporterObj::free_import_info(AssetImportInfo* info)
{
    sBuilderMeta[(int)info->type].free_info(info);
}

AssetImportJob* AssetImporterObj::allocate_import_job(AssetType type)
{
    AssetImportJob* job = (AssetImportJob*)importJobPA.allocate();
    new (job) AssetImportJob();

    job->info = nullptr;

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
        status.str = "destination path is empty";
    }
    else if (info->dstURI.empty())
    {
        status.type = ASSET_IMPORT_ERROR_DST_URI;
        status.str = "requested URI is empty";
    }
    else if (FS::exists(info->dstPath))
    {
        status.type = ASSET_IMPORT_ERROR_DST_PATH;
        status.str = "destination file already exists";
    }
    else if (registry && registry.get_entry_by_uri(info->dstURI))
    {
        status.type = ASSET_IMPORT_ERROR_DST_URI;
        status.str = "URI already registered in project";
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

void AssetImporter::set_suid_registry(SUIDRegistry idReg)
{
    mObj->suidRegistry = idReg;
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
                job->status = mObj->resolve_asset(job->asset, job->info);

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
    job->info = info;

    if (job->status)
        job->execute_synchronous();
    else
        job->hasCompleted.store(true);

    LD_ASSERT(job->has_completed());

    AssetImportResult result{};
    result.dstAsset = job->asset;
    result.status = job->status;

    if (job->status)
        job->status = mObj->resolve_asset(job->asset, job->info);

    mObj->free_import_job(job);

    return result;
}

} // namespace LD