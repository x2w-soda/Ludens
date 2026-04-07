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

    void load_project_scene(const FS::Path& sceneSchemaPath);
};

void ProjectLoadAsyncObj::load_project_scene(const FS::Path& sceneSchemaPath)
{
    LD_PROFILE_SCOPE;

    if (!FS::exists(sceneSchemaPath))
        return;

    Scene scene = Scene::get();
    LD_ASSERT(scene);
    scene.unload();

    scene.load([&](SceneObj* sceneObj) -> bool {
        // load the scene
        std::string err;
        return SceneSchema::load_scene_from_file(Scene(sceneObj), projectCtx->suid_registry(), sceneSchemaPath, err);
    });

    // TODO: check scene load success
    this->sceneSchemaAbsPath = sceneSchemaPath;
}

ProjectLoadAsync ProjectLoadAsync::create(ProjectContext* projectCtx)
{
    auto* obj = heap_new<ProjectLoadAsyncObj>(MEMORY_USAGE_MISC);

    obj->projectCtx = projectCtx;

    return ProjectLoadAsync(obj);
}

void ProjectLoadAsync::destroy(ProjectLoadAsync async)
{
    auto* obj = async.unwrap();

    LD_ASSERT(obj->status == PROJECT_LOAD_STATE_IDLE);

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
    if (!mObj->projectCtx->load_project(projectSchemaAbsPath, err))
        return false;

    sLog.info("loaded project schema [{}]", projectSchemaAbsPath.string());

    FS::Path assetSchemaAbsPath = mObj->projectCtx->asset_schema_abs_path();
    if (!FS::exists(assetSchemaAbsPath))
    {
        sLog.error("missing asset schema file [{}]", assetSchemaAbsPath.string());
        return false;
    }

    if (!mObj->projectCtx->load_asset_registry(assetSchemaAbsPath, err))
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
        Vector<std::string> errors;
        if (!AM.end_load_batch(errors))
        {
            sLog.warn("AssetManager failed to load some assets, {} errors", errors.size());
            for (const std::string& err : errors)
                sLog.warn("{}", err);
        }

        Vector<FS::Path> scenePaths = mObj->projectCtx->scene_schema_abs_paths();

        for (const FS::Path& scenePath : scenePaths)
        {
            if (!FS::exists(scenePath))
            {
                sLog.error("- missing scene {}", scenePath.string());
                continue;
            }

            sLog.info("- found scene {}", scenePath.string());
        }

        if (!scenePaths.empty())
            mObj->load_project_scene(scenePaths.front());

        mObj->status = PROJECT_LOAD_STATE_COMPLETE;
    }

    return mObj->status;
}

bool ProjectLoadAsync::get_result(ProjectLoadResult& outResult)
{
    if (mObj->status != PROJECT_LOAD_STATE_COMPLETE)
        return false;

    outResult.sceneSchemaAbsPath = mObj->sceneSchemaAbsPath;
    mObj->sceneSchemaAbsPath.clear();

    mObj->status = PROJECT_LOAD_STATE_IDLE;
    return true;
}

} // namespace LD