#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/ProjectLoadAsync.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/Scene/SceneSchema.h>

namespace LD {

static Log sLog("ProjectLoadAsync");

struct ProjectLoadAsyncObj
{
    FS::Path projectDir;
    FS::Path assetSchemaPath;
    FS::Path sceneSchemaPath;
    Project project;
    AssetRegistry assetRegistry;
    ProjectLoadStatus status = PROJECT_LOAD_STATUS_IDLE;

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
        return SceneSchema::load_scene_from_file(Scene(sceneObj), sceneSchemaPath, err);
    });

    // TODO: check scene load success
    this->sceneSchemaPath = sceneSchemaPath;
}

ProjectLoadAsync ProjectLoadAsync::create()
{
    auto* obj = heap_new<ProjectLoadAsyncObj>(MEMORY_USAGE_MISC);

    return ProjectLoadAsync(obj);
}

void ProjectLoadAsync::destroy(ProjectLoadAsync async)
{
    auto* obj = async.unwrap();

    LD_ASSERT(obj->status == PROJECT_LOAD_STATUS_IDLE);

    heap_delete<ProjectLoadAsyncObj>(obj);
}

bool ProjectLoadAsync::begin(const FS::Path& projectDir, std::string& err)
{
    // This synchronously reads ProjectSchema and AssetSchema on main thread
    // and submits asset load jobs for all assets in project before returning.
    LD_PROFILE_SCOPE;

    mObj->status = PROJECT_LOAD_STATUS_IDLE;
    mObj->projectDir = projectDir;
    mObj->project = Project::create();

    FS::Path projectSchemaPath = FS::absolute(projectDir / FS::Path("project.toml"));
    if (!ProjectSchema::load_project_from_file(mObj->project, projectSchemaPath, err))
        return false;

    sLog.info("loaded project schema [{}]", projectSchemaPath.string());

    FS::Path assetSchemaPath = mObj->project.get_asset_schema_absolute_path();
    if (!FS::exists(assetSchemaPath))
    {
        sLog.error("missing asset schema file [{}]", assetSchemaPath.string());
        return false;
    }

    mObj->assetSchemaPath = assetSchemaPath;

    mObj->assetRegistry = AssetRegistry::create();
    if (!AssetSchema::load_registry_from_file(mObj->assetRegistry, assetSchemaPath, err))
        return false;

    sLog.info("loaded asset schema   [{}]", assetSchemaPath.string());

    AssetManager AM = AssetManager::get();
    LD_ASSERT(AM);

    AssetRegistry oldAssetRegistry = AM.swap_asset_registry(mObj->assetRegistry, mObj->projectDir);
    AM.begin_load_batch();
    AM.load_all_assets();

    if (oldAssetRegistry)
        AssetRegistry::destroy(oldAssetRegistry);

    mObj->status = PROJECT_LOAD_STATUS_LOADING_ASSETS;

    return true;
}

ProjectLoadStatus ProjectLoadAsync::update()
{
    AssetManager AM = AssetManager::get();

    if (mObj->status == PROJECT_LOAD_STATUS_LOADING_ASSETS)
    {
        if (!AM.has_load_job())
            mObj->status = PROJECT_LOAD_STATUS_LOADING_SCENE;
    }

    if (mObj->status == PROJECT_LOAD_STATUS_LOADING_SCENE)
    {
        Vector<std::string> errors;
        if (!AM.end_load_batch(errors))
        {
            sLog.warn("AssetManager failed to load some assets, {} errors", errors.size());
            for (const std::string& err : errors)
                sLog.warn("{}", err);
        }

        Vector<FS::Path> scenePaths;
        mObj->project.get_scene_schema_absolute_paths(scenePaths);

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

        mObj->status = PROJECT_LOAD_STATUS_COMPLETE;
    }

    return mObj->status;
}

bool ProjectLoadAsync::get_result(ProjectLoadResult& outResult)
{
    if (mObj->status != PROJECT_LOAD_STATUS_COMPLETE)
        return false;

    outResult.assetRegistry = mObj->assetRegistry;
    mObj->assetRegistry = {};

    outResult.project = mObj->project;
    mObj->project = {};

    outResult.assetSchemaPath = mObj->assetSchemaPath;
    mObj->assetSchemaPath.clear();

    outResult.sceneSchemaPath = mObj->sceneSchemaPath;
    mObj->sceneSchemaPath.clear();

    mObj->status = PROJECT_LOAD_STATUS_IDLE;
    return true;
}

} // namespace LD