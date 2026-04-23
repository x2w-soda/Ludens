#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Platform.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/ProjectContext.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/FileSystemAsync.h>
#include <LudensBuilder/ProjectBuilder/ProjectBuilder.h>

#include <EmbedRuntime.h>

namespace LD {

static Log sLog("ProjectBuilder");

enum AsyncStatus : uint32_t
{
    ASYNC_STATUS_IDLE = 0,
    ASYNC_STATUS_IN_PROGRESS = 1,
    ASYNC_STATUS_SUCCESS = 2,
    ASYNC_STATUS_FAILURE = 3,
};

class WriteFileJob
{
public:
    void submit(const FS::Path& dstPath, View srcView)
    {
        mSrcView = srcView;
        mDstPath = dstPath;
        mJob.user = this;
        mJob.onExecute = &WriteFileJob::execute;
        JobSystem::get().submit(&mJob, JOB_DISPATCH_STANDARD);
    }

    bool has_completed(bool& success)
    {
        size_t bytesWritten;
        return mAsync.has_completed(success, bytesWritten);
    }

private:
    static void execute(void* user)
    {
        LD_PROFILE_SCOPE;

        auto* obj = (WriteFileJob*)user;
        obj->mAsync.begin(obj->mDstPath, obj->mSrcView);
    }

    JobHeader mJob{};
    View mSrcView;
    FS::Path mDstPath;
    FS::WriteFileAsync mAsync;
};

class CopyFileJob
{
public:
    FS::Path srcPath;
    FS::Path dstPath;

public:
    void submit()
    {
        mJob.user = this;
        mJob.onExecute = &CopyFileJob::execute;
        JobSystem::get().submit(&mJob, JOB_DISPATCH_STANDARD);
    }

    bool has_completed(bool& success)
    {
        AsyncStatus status = (AsyncStatus)mStatus.load();

        switch (status)
        {
        case ASYNC_STATUS_IDLE:
        case ASYNC_STATUS_IN_PROGRESS:
            return false;
        case ASYNC_STATUS_SUCCESS:
            success = true;
            break;
        case ASYNC_STATUS_FAILURE:
            success = false;
            break;
        }

        return true;
    }

private:
    static void execute(void* user)
    {
        LD_PROFILE_SCOPE;

        auto* obj = (CopyFileJob*)user;

        obj->mStatus.store(ASYNC_STATUS_IN_PROGRESS);
        bool success = FS::copy_file(obj->srcPath, obj->dstPath, FS::COPY_OPTION_OVERWRITE_EXISTING_BIT, obj->mError);
        if (!success)
            sLog.error("{}", obj->mError);
        obj->mStatus.store(success ? ASYNC_STATUS_SUCCESS : ASYNC_STATUS_FAILURE);
    }

    JobHeader mJob{};
    std::string mError;
    std::atomic_uint32_t mStatus{ASYNC_STATUS_IDLE};
};

struct ProjectBuildAsyncObj
{
    ProjectContext projectCtx = {};
    ProjectBuildConfig config{};
    ProjectBuildResult result{};
    WriteFileJob writeRuntimeFileJob;
    WriteFileJob writeProjectSchemaJob;
    WriteFileJob writeAssetSchemaJob;
    Vector<CopyFileJob*> copyAssetJobs;
    Vector<CopyFileJob*> copySceneSchemaJobs;
    std::string dstAssetSchemaTOML;
    std::string dstProjectSchemaTOML;
    bool hasCompleted;

    bool load_project_schema(const FS::Path& srcProjectSchema, ProjectBuildStatus& err);
    bool load_asset_schema(const FS::Path& srcAssetSchema, ProjectBuildStatus& err);
    bool configure_dst_project_schema(ProjectBuildStatus& err);
    bool configure_dst_asset_schema(ProjectBuildStatus& err);
    bool begin(const ProjectBuildConfig& cfg, ProjectBuildStatus& err);
};

bool ProjectBuildAsyncObj::load_project_schema(const FS::Path& srcProjectSchema, ProjectBuildStatus& err)
{
    if (!projectCtx.load_project_schema(srcProjectSchema, err.str))
    {
        err.type = PROJECT_BUILD_ERROR_INVALID_SRC_PROJECT_SCHEMA;
        sLog.error("failed to load project {}: {}", srcProjectSchema.string(), err.str);
        return false;
    }

    if (!FS::create_directories(config.dstRootDirectory, err.str))
    {
        err.type = PROJECT_BUILD_ERROR_IO;
        return false;
    }

    return true;
}

bool ProjectBuildAsyncObj::load_asset_schema(const FS::Path& srcAssetSchema, ProjectBuildStatus& err)
{
    if (!projectCtx.load_asset_schema(srcAssetSchema, err.str))
    {
        err.type = PROJECT_BUILD_ERROR_INVALID_SRC_ASSET_SCHEMA;
        sLog.error("failed to load asset schema {}: {}", srcAssetSchema.string(), err.str);
        return false;
    }

    return true;
}

bool ProjectBuildAsyncObj::configure_dst_project_schema(ProjectBuildStatus& err)
{
    Project project = projectCtx.project();
    const FS::Path srcRootDirAbsPath = project.get_root_dir_abs_path();
    const FS::Path dstRootDirAbsPath = FS::absolute(config.dstRootDirectory);
    const FS::Path dstStorageDirAbsPath = FS::absolute(dstRootDirAbsPath / project.get_storage_dir_rel_path());

    Vector<SUIDEntry> scenes;
    project.get_scenes(scenes);
    copySceneSchemaJobs.resize(scenes.size());

    if (!FS::create_directories(dstStorageDirAbsPath, err.str))
    {
        err.type = PROJECT_BUILD_ERROR_IO;
        return false;
    }

    // flatten scenes in build dst directory
    for (size_t i = 0; i < scenes.size(); i++)
    {
        const SUIDEntry& entry = scenes[i];
        FS::Path relPath;
        (void)project.get_scene_schema_rel_path(entry.id, relPath);

        // absolute paths for copy job
        copySceneSchemaJobs[i] = heap_new<CopyFileJob>(MEMORY_USAGE_MISC);
        copySceneSchemaJobs[i]->srcPath = srcRootDirAbsPath / relPath;
        copySceneSchemaJobs[i]->dstPath = dstRootDirAbsPath / relPath;
    }

    if (!ProjectSchema::save_project_to_string(project, dstProjectSchemaTOML, err.str))
    {
        // TODO:
        return false;
    }

    return true;
}

bool ProjectBuildAsyncObj::configure_dst_asset_schema(ProjectBuildStatus& err)
{
    Project project = projectCtx.project();
    const FS::Path dstAssetDirectory("Assets");
    const FS::Path srcRootDirectory = project.get_root_dir_abs_path();

    // temporary AssetRegistry that we can patch safely.
    AssetRegistry assetRegistry = projectCtx.asset_registry();
    Vector<std::string> keys;
    Vector<AssetEntry> assets;
    assetRegistry.get_all_entries(assets);
    copyAssetJobs.clear();
    copyAssetJobs.reserve(assets.size());

    if (!FS::create_directories(config.dstRootDirectory / dstAssetDirectory, err.str))
    {
        err.type = PROJECT_BUILD_ERROR_IO;
        return false;
    }

    // flatten assets in build dst directory, copy files over
    for (AssetEntry entry : assets)
    {
        keys = entry.get_file_path_keys();
        for (const std::string& key : keys)
        {
            FS::Path srcAssetPath = FS::Path(entry.get_file_path(key));
            FS::Path dstAssetPath = dstAssetDirectory / srcAssetPath.filename();

            CopyFileJob* job = heap_new<CopyFileJob>(MEMORY_USAGE_MISC);
            job->srcPath = srcRootDirectory / srcAssetPath;
            job->dstPath = config.dstRootDirectory / dstAssetPath;
            copyAssetJobs.push_back(job);

            entry.set_file_path(key, dstAssetPath.string());
        }
    }

    if (!AssetSchema::save_registry_to_string(assetRegistry, dstAssetSchemaTOML, err.str))
    {
        // TODO:
        return false;
    }

    return true;
}

bool ProjectBuildAsyncObj::begin(const ProjectBuildConfig& cfg, ProjectBuildStatus& err)
{
    LD_PROFILE_SCOPE;

    config = cfg;

    if (!load_project_schema(config.srcProjectSchema, err))
        return false;

    std::string projectName = projectCtx.project().get_name();
    std::replace(projectName.begin(), projectName.end(), ' ', '_');
    FS::Path projectPath(projectName);

#ifdef LD_PLATFORM_WIN32
    projectPath.replace_extension(".exe");
#endif

    const FS::Path dstRuntimePath = config.dstRootDirectory / projectPath;
    const FS::Path srcProjectSchemaPath = projectCtx.project_schema_abs_path();
    const FS::Path dstProjectSchemaPath = config.dstRootDirectory / FS::Path("project.toml");
    const FS::Path srcAssetSchemaPath = projectCtx.asset_schema_abs_path();
    const FS::Path dstAssetSchemaPath = config.dstRootDirectory / projectCtx.project().get_asset_schema_rel_path();

    if (!load_asset_schema(srcAssetSchemaPath, err))
        return false;

    if (!configure_dst_project_schema(err))
        return false;

    if (!configure_dst_asset_schema(err))
        return false;

    // TODO: validate all src schema and src files are coherent
    //       before firing off jobs.

    sLog.debug("Begin copy runtime to [{}]", dstRuntimePath.string());
    writeRuntimeFileJob.submit(dstRuntimePath, View((const char*)EmbedRuntimeData, EmbedRuntimeSize));

    sLog.debug("Begin copy project schema to [{}]", dstProjectSchemaPath.string());
    writeProjectSchemaJob.submit(dstProjectSchemaPath, View(dstProjectSchemaTOML.data(), dstProjectSchemaTOML.size()));

    sLog.debug("Begin copy asset schema to [{}]", dstAssetSchemaPath.string());
    writeAssetSchemaJob.submit(dstAssetSchemaPath, View(dstAssetSchemaTOML.data(), dstAssetSchemaTOML.size()));

    for (CopyFileJob* job : copyAssetJobs)
        job->submit();

    for (CopyFileJob* job : copySceneSchemaJobs)
        job->submit();

    result.reset();
    hasCompleted = false;
    return true;
}

//
// Public API
//

void ProjectBuildResult::reset()
{
    success = false;
}

ProjectBuildAsync ProjectBuildAsync::create()
{
    // TODO:
    LD_UNREACHABLE;

    auto* obj = heap_new<ProjectBuildAsyncObj>(MEMORY_USAGE_MISC);

    return ProjectBuildAsync(obj);
}

void ProjectBuildAsync::destroy(ProjectBuildAsync async)
{
    auto* obj = async.unwrap();

    for (CopyFileJob* job : obj->copyAssetJobs)
        heap_delete<CopyFileJob>(job);

    for (CopyFileJob* job : obj->copySceneSchemaJobs)
        heap_delete<CopyFileJob>(job);

    heap_delete<ProjectBuildAsyncObj>(obj);
}

bool ProjectBuildAsync::begin(const ProjectBuildConfig& config, ProjectBuildStatus& err)
{
    return mObj->begin(config, err);
}

bool ProjectBuildAsync::has_completed()
{
    if (mObj->hasCompleted)
        return true;

    bool success = true;
    bool jobSuccess;

    if (!mObj->writeRuntimeFileJob.has_completed(jobSuccess))
        return false;

    success = success && jobSuccess;

    if (!mObj->writeProjectSchemaJob.has_completed(jobSuccess))
        return false;

    success = success && jobSuccess;

    if (!mObj->writeAssetSchemaJob.has_completed(jobSuccess))
        return false;

    for (CopyFileJob* job : mObj->copyAssetJobs)
    {
        if (!job->has_completed(jobSuccess))
            return false;

        success = success && jobSuccess;
    }

    for (CopyFileJob* job : mObj->copySceneSchemaJobs)
    {
        if (!job->has_completed(jobSuccess))
            return false;

        success = success && jobSuccess;
    }

    // all completed
    mObj->result.success = success;

    return mObj->hasCompleted = true;
}

bool ProjectBuildAsync::get_result(ProjectBuildResult& outResult)
{
    if (!mObj->hasCompleted)
        return false;

    outResult = mObj->result;
    return true;
}

bool create_empty_project(const std::string& projectName, const FS::Path& projectSchema, std::string& err)
{
    LD_PROFILE_SCOPE;

    const FS::Path projectDir = projectSchema.parent_path();

    if (!FS::create_directories(projectDir, err))
    {
        err = std::format("failed to create destination directory [{}]", projectDir.string());
        return false;
    }

    std::string toml;

    toml = AssetSchema::create_empty();
    FS::Path assetSchemaPath = projectDir / FS::Path("assets.toml");
    if (!FS::write_file(assetSchemaPath, View(toml.data(), toml.size()), err))
        return false;

    toml = ProjectSchema::create_empty(projectName, "assets.toml", "main.toml", "main");
    FS::Path projectSchemaPath = projectDir / FS::Path("project.toml");
    if (!FS::write_file(projectSchemaPath, View(toml.data(), toml.size()), err))
        return false;

    toml = SceneSchema::create_empty();
    FS::Path sceneSchemaPath = projectDir / FS::Path("main.toml");
    if (!FS::write_file(sceneSchemaPath, View(toml.data(), toml.size()), err))
        return false;

    return true;
}

} // namespace LD
