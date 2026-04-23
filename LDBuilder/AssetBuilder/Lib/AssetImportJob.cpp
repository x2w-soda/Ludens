#include <Ludens/JobSystem/JobSystem.h>
#include <LudensBuilder/AssetBuilder/AssetSource/AssetSources.h>

#include "AssetBuilderMeta.h"
#include "AssetImportJob.h"

namespace LD {

void AssetImportJob::submit()
{
    status.type = ASSET_IMPORT_SUCCESS;
    status.str.clear();

    JobHeader header{};
    header.onExecute = sImportMeta[info->type].importFn;
    header.onComplete = [](void* user) { ((AssetImportJob*)user)->hasCompleted.store(true, std::memory_order_release); };
    header.user = this;
    JobSystem::get().submit(&header, JOB_DISPATCH_STANDARD);
}

void AssetImportJob::execute_synchronous()
{
    status.type = ASSET_IMPORT_SUCCESS;
    status.str.clear();

    sImportMeta[(int)info->type].importFn(this);

    hasCompleted.store(true, std::memory_order_release);
}

} // namespace LD