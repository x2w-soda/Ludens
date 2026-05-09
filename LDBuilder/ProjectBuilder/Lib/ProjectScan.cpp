#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/FileSystem.h>
#include <LudensBuilder/ProjectBuilder/ProjectScan.h>

#include <atomic>

namespace LD {

struct ProjectScanAsyncObj
{
    ProjectScanResult result;
    std::atomic<ProjectScanStatus> status = PROJECT_SCAN_STATUS_IDLE;

    static void load_project_schema(void* user);
};

void ProjectScanAsyncObj::load_project_schema(void* user)
{
    LD_PROFILE_SCOPE;

    ProjectScanAsyncObj* obj = (ProjectScanAsyncObj*)user;
    SUIDRegistry suidReg = SUIDRegistry::create();
    Project project = Project::create();

    String err;
    obj->result.isProjectSchemaValid = ProjectSchema::load_project_from_file(project, suidReg, obj->result.projectSchema, err);
    if (obj->result.isProjectSchemaValid)
    {
        obj->result.projectName = project.get_name();
    }

    Project::destroy(project);
    SUIDRegistry::destroy(suidReg);
    obj->status.store(PROJECT_SCAN_STATUS_COMPLETE);
}

ProjectScanAsync ProjectScanAsync::create()
{
    auto* obj = heap_new<ProjectScanAsyncObj>(MEMORY_USAGE_MISC);

    return ProjectScanAsync(obj);
}

void ProjectScanAsync::destroy(ProjectScanAsync async)
{
    auto* obj = async.unwrap();

    heap_delete<ProjectScanAsyncObj>(obj);
}

void ProjectScanAsync::begin(const FS::Path& projectSchema)
{
    mObj->status.store(PROJECT_SCAN_STATUS_PROJECT_SCHEMA);
    mObj->result = {};
    mObj->result.projectSchema = FS::absolute(projectSchema);

    // main thread should not read from result until scan completion
    JobHeader job{};
    job.user = mObj;
    job.onExecute = ProjectScanAsyncObj::load_project_schema;
    JobSystem::get().submit(&job, JOB_DISPATCH_STANDARD);
}

ProjectScanStatus ProjectScanAsync::update()
{
    return mObj->status.load();
}

bool ProjectScanAsync::get_result(ProjectScanResult& outResult)
{
    if (mObj->status.load() != PROJECT_SCAN_STATUS_COMPLETE)
        return false;

    mObj->status.store(PROJECT_SCAN_STATUS_IDLE); // consume result
    outResult = mObj->result;
    return true;
}

} // namespace LD