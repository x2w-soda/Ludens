#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/ProjectContext.h>
#include <Ludens/Project/ProjectLoadAsync.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/Scene/SceneSchema.h>

namespace LD {

static Log sLog("ProjectLoadAsync");

struct ProjectLoadAsyncObj
{
    FS::Path projectDir;
    FS::Path sceneSchemaAbsPath;
    ProjectContext* projectCtx = nullptr;
    ProjectLoadState status = PROJECT_LOAD_STATE_IDLE;
};

ProjectLoadAsync ProjectLoadAsync::create(ProjectContext* projectCtx)
{
    auto* obj = heap_new<ProjectLoadAsyncObj>(MEMORY_USAGE_MISC);

    obj->projectCtx = projectCtx;

    return ProjectLoadAsync(obj);
}

void ProjectLoadAsync::destroy(ProjectLoadAsync async)
{
    auto* obj = async.unwrap();

    LD_ASSERT(obj->status == PROJECT_LOAD_STATE_IDLE || obj->status == PROJECT_LOAD_STATE_COMPLETE);

    heap_delete<ProjectLoadAsyncObj>(obj);
}

bool ProjectLoadAsync::begin(const FS::Path& projectDir, std::string& err)
{
    // This synchronously reads ProjectSchema and AssetSchema on main thread
    // and submits asset load jobs for all assets in project before returning.
    LD_PROFILE_SCOPE;

    mObj->status = PROJECT_LOAD_STATE_IDLE;
    mObj->projectDir = projectDir;

    FS::Path projectSchemaAbsPath = FS::absolute(projectDir / FS::Path("project.toml"));
    if (!mObj->projectCtx->load_project_schema(projectSchemaAbsPath, err))
        return false;

    sLog.info("loaded project schema [{}]", projectSchemaAbsPath.string());

    FS::Path assetSchemaAbsPath = mObj->projectCtx->asset_schema_abs_path();
    if (!FS::exists(assetSchemaAbsPath))
    {
        sLog.error("missing asset schema file [{}]", assetSchemaAbsPath.string());
        return false;
    }

    if (!mObj->projectCtx->load_asset_schema(assetSchemaAbsPath, err))
        return false;

    sLog.info("loaded asset schema   [{}]", assetSchemaAbsPath.string());

    AssetManager AM = AssetManager::get();
    LD_ASSERT(AM);

    AssetRegistry oldAssetRegistry = AM.swap_asset_registry(mObj->projectCtx->asset_registry(), mObj->projectDir);
    AM.begin_load_batch();
    AM.load_all_assets();

    if (oldAssetRegistry)
        AssetRegistry::destroy(oldAssetRegistry);

    mObj->status = PROJECT_LOAD_STATE_LOADING_ASSETS;

    return true;
}

ProjectLoadState ProjectLoadAsync::update()
{
    AssetManager AM = AssetManager::get();

    if (mObj->status == PROJECT_LOAD_STATE_LOADING_ASSETS)
    {
        if (!AM.has_load_job())
            mObj->status = PROJECT_LOAD_STATE_LOADING_SCENE;
    }

    if (mObj->status == PROJECT_LOAD_STATE_LOADING_SCENE)
    {
        Vector<AssetLoadStatus> errors;
        if (!AM.end_load_batch(errors))
        {
            sLog.warn("AssetManager failed to load some assets, {} errors", errors.size());
            for (const AssetLoadStatus& err : errors)
                sLog.warn("{}", err.str);
        }

        Vector<FS::Path> scenePaths;
        mObj->projectCtx->project().get_scene_schema_abs_paths(scenePaths);

        for (const FS::Path& scenePath : scenePaths)
        {
            if (!FS::exists(scenePath))
            {
                sLog.error("- missing scene {}", scenePath.string());
                continue;
            }

            sLog.info("- found scene {}", scenePath.string());
        }

        mObj->status = PROJECT_LOAD_STATE_COMPLETE;
    }

    return mObj->status;
}

} // namespace LD