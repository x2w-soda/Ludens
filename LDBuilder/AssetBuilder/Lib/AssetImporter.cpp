#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Memory/Allocator.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>

#include "AssetImportJob.h"

#define LD_ASSERT_IMPORT_SCOPE LD_ASSERT(mObj->isBatchScope)
#define LD_ASSERT_NOT_IMPORT_SCOPE LD_ASSERT(!mObj->isBatchScope)

namespace LD {

struct AssetImporterObj
{
    AssetRegistry registry = {};             // registry to query
    PoolAllocator importJobPA = {};          // pool allocator for import jobs
    Vector<AssetImportJob*> importJobs = {}; // all jobs in a batch
    bool isBatchScope = false;

    AssetImportJob* allocate_import_job(AssetType type);
    void free_import_job(AssetImportJob*);
};

AssetImportJob* AssetImporterObj::allocate_import_job(AssetType type)
{
    AssetImportJob* job = (AssetImportJob*)importJobPA.allocate();
    new (job) AssetImportJob();

    job->info.startup(type);

    return job;
}

void AssetImporterObj::free_import_job(AssetImportJob* job)
{
    job->info.cleanup();

    job->~AssetImportJob();
    importJobPA.free(job);
}

//
// Public API
//

AssetImporter AssetImporter::create()
{
    auto* obj = heap_new<AssetImporterObj>(MEMORY_USAGE_ASSET);

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(AssetImportInfoStorage);
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

void AssetImporter::import_batch_begin()
{
    LD_ASSERT_NOT_IMPORT_SCOPE;

    mObj->isBatchScope = true;
}

void AssetImporter::import_batch_end(Vector<AssetImportResult>& outResults)
{
    LD_ASSERT_IMPORT_SCOPE;

    // TODO: wait on import job types
    JobSystem::get().wait_all();

    mObj->isBatchScope = false;

    outResults.resize(mObj->importJobs.size());

    for (size_t i = 0; i < mObj->importJobs.size(); i++)
    {
        AssetImportJob* job = mObj->importJobs[i];
        LD_ASSERT(job->has_completed());

        outResults[i].status = job->status;
        outResults[i].dstAsset = job->asset;
    }

    // TODO:
}

bool AssetImporter::import_batch_update(AssetImportResult& outResult)
{
    LD_ASSERT_IMPORT_SCOPE;

    AssetImportJob* completedJob = nullptr;

    for (AssetImportJob* job : mObj->importJobs)
    {
        if (job->has_completed())
        {
            completedJob = job;
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

void AssetImporter::import_batch_asset(Asset dstAsset, const AssetImportInfo* info)
{
    LD_ASSERT_IMPORT_SCOPE;

    AssetImportJob* job = mObj->allocate_import_job(dstAsset.get_type());
    mObj->importJobs.push_back(job);

    // TODO: query AssetRegistry
    //       - check URI conflict
    //       - check dst file conflict

    job->asset = dstAsset;
    job->copy_info(info);
    job->submit();
}

AssetImportResult AssetImporter::import_asset_synchronous(Asset dstAsset, const AssetImportInfo* info)
{
    LD_ASSERT_NOT_IMPORT_SCOPE;

    AssetImportJob* job = mObj->allocate_import_job(dstAsset.get_type());

    job->execute_synchronous();
    LD_ASSERT(job->has_completed());

    AssetImportResult result{};
    result.dstAsset = job->asset;
    result.status = job->status;

    mObj->free_import_job(job);

    return result;
}

} // namespace LD