#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <utility>

namespace LD {

static Log sLog("EditorContext");

using EditorContextObserver = std::pair<EditorContextEventFn, void*>;

/// @brief Editor context implementation. Keeps track of the active project
///        and active scene states.
struct EditorContextObj
{
    RServer renderServer;             /// render server handle
    RImage iconAtlas;                 /// editor icon atlas handle
    Project project;                  /// current project under edit
    Scene scene;                      /// current scene under edit
    AssetManager assetManager;        /// loads assets for the scene
    EditorSettings settings;          /// editor global settings
    FS::Path iconAtlasPath;           /// path to editor icon atlas source file
    FS::Path sceneTOMLPath;           /// path to current scene file
    FS::Path assetTOMLPath;           /// path to project asset file
    FS::Path projectDirPath;          /// path to project root directory
    std::string projectName;          /// project identifier
    std::vector<FS::Path> scenePaths; /// path to scene files in project
    std::vector<EditorContextObserver> observers;
    CUID selectedComponent;
    RUID selectedComponentRUID;
    bool isPlaying;

    void notify_observers(const EditorContextEvent* event);
};

void EditorContextObj::notify_observers(const EditorContextEvent* event)
{
    for (auto& observer : observers)
    {
        observer.first(event, observer.second);
    }
}

EditorContext EditorContext::create(const EditorContextInfo& info)
{
    EditorContextObj* obj = heap_new<EditorContextObj>(MEMORY_USAGE_MISC);
    obj->renderServer = info.renderServer;
    obj->iconAtlasPath = info.iconAtlasPath;
    obj->settings = EditorSettings::create_default();
    obj->isPlaying = false;

    return {obj};
}

void EditorContext::destroy(EditorContext ctx)
{
    EditorContextObj* obj = ctx;

    if (obj->iconAtlas)
    {
        RDevice device = obj->renderServer.get_device();
        device.wait_idle();
        device.destroy_image(obj->iconAtlas);
    }

    if (obj->assetManager)
    {
        AssetManager::destroy(obj->assetManager);
        obj->assetManager = {};
    }

    Project::destroy(obj->project);
    Scene::destroy(obj->scene);
    EditorSettings::destroy(obj->settings);

    heap_delete<EditorContextObj>(obj);
}

Mat4 EditorContext::render_server_transform_callback(RUID ruid, void* user)
{
    EditorContextObj& self = *(EditorContextObj*)user;

    return self.scene.get_ruid_transform_mat4(ruid);
}

EditorSettings EditorContext::get_settings()
{
    return mObj->settings;
}

AssetManager EditorContext::get_asset_manager()
{
    return mObj->assetManager;
}

RImage EditorContext::get_editor_icon_atlas()
{
    RDevice device = mObj->renderServer.get_device();

    if (!mObj->iconAtlas)
    {
        std::string iconAtlasPath = mObj->iconAtlasPath.string();
        Bitmap tmpBitmap = Bitmap::create_from_path(iconAtlasPath.c_str(), false);
        RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT,
                                                      RFORMAT_RGBA8, tmpBitmap.width(), tmpBitmap.height(),
                                                      {.filter = RFILTER_LINEAR, .mipmapFilter = RFILTER_LINEAR, .addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE});

        mObj->iconAtlas = device.create_image(imageI);

        RStager stager(device, RQUEUE_TYPE_GRAPHICS);
        stager.add_image_data(mObj->iconAtlas, tmpBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
        stager.submit(device.get_graphics_queue());
        Bitmap::destroy(tmpBitmap);
    }

    return mObj->iconAtlas;
}

void EditorContext::add_observer(EditorContextEventFn fn, void* user)
{
    mObj->observers.push_back(std::make_pair(fn, user));
}

void EditorContext::update(float delta)
{
    if (mObj->isPlaying)
    {
        mObj->scene.update(delta);
    }
}

void EditorContext::load_project(const std::filesystem::path& filePath)
{
    LD_PROFILE_SCOPE;

    FS::Path projectDirPath = filePath.parent_path();

    TOMLDocument projectDoc = TOMLDocument::create_from_file(filePath);
    mObj->project = Project::create(projectDirPath);
    ProjectSchema::load_project(mObj->project, projectDoc);
    TOMLDocument::destroy(projectDoc);

    mObj->projectDirPath = projectDirPath;
    mObj->projectName = mObj->project.get_name();

    FS::Path assetsPath = mObj->project.get_assets_path();
    sLog.info("loading project [{}], root directory {}", mObj->projectName, mObj->projectDirPath.string());

    mObj->assetTOMLPath = assetsPath;

    if (!FS::exists(mObj->assetTOMLPath))
    {
        sLog.warn("failed to find project assets {}", mObj->assetTOMLPath.string());
        return;
    }

    if (mObj->assetManager)
    {
        AssetManager::destroy(mObj->assetManager);
    }

    // Load all project assets at once using job system.
    // Once we have asynchronous-load-jobs maybe we can load assets
    // used by the loaded scene first?
    TOMLDocument assetDoc = TOMLDocument::create_from_file(mObj->assetTOMLPath);
    mObj->assetManager = AssetManager::create(mObj->projectDirPath);
    AssetSchema::load_assets(mObj->assetManager, assetDoc);
    TOMLDocument::destroy(assetDoc);

    mObj->project.get_scene_paths(mObj->scenePaths);

    for (const FS::Path& scenePath : mObj->scenePaths)
    {
        if (!FS::exists(scenePath))
        {
            sLog.error("- missing scene {}", scenePath.string());
            continue;
        }

        sLog.info("- found scene {}", scenePath.string());
    }

    if (!mObj->scenePaths.empty())
        load_project_scene(mObj->scenePaths.front());

    EditorContextProjectLoadEvent event{};
    mObj->notify_observers(&event);
}

void EditorContext::load_project_scene(const std::filesystem::path& tomlPath)
{
    LD_PROFILE_SCOPE;

    mObj->sceneTOMLPath = tomlPath;
    mObj->selectedComponent = 0;
    mObj->selectedComponentRUID = 0;

    // create the scene
    TOMLDocument tomlDoc = TOMLDocument::create_from_file(mObj->sceneTOMLPath);
    mObj->scene = Scene::create();
    SceneSchema::load_scene(mObj->scene, tomlDoc);
    TOMLDocument::destroy(tomlDoc);

    // prepare the scene
    ScenePrepareInfo prepareInfo{};
    prepareInfo.assetManager = mObj->assetManager;
    prepareInfo.renderServer = mObj->renderServer;
    mObj->scene.prepare(prepareInfo);

    EditorContextSceneLoadEvent event{};
    mObj->notify_observers(&event);
}

void EditorContext::play_scene()
{
    LD_PROFILE_SCOPE;

    if (mObj->isPlaying)
        return;

    mObj->isPlaying = true;

    // play a duplicated scene
    mObj->scene.backup();
    mObj->scene.swap();
    mObj->scene.startup();
}

void EditorContext::stop_scene()
{
    LD_PROFILE_SCOPE;

    if (!mObj->isPlaying)
        return;

    mObj->isPlaying = false;

    // restore original scene
    mObj->scene.cleanup();
    mObj->scene.swap();
}

bool EditorContext::is_playing()
{
    return mObj->isPlaying;
}

void EditorContext::get_scene_roots(std::vector<CUID>& roots)
{
    mObj->scene.get_root_components(roots);
}

const ComponentBase* EditorContext::get_component_base(CUID comp)
{
    return mObj->scene.get_component_base(comp);
}

const char* EditorContext::get_component_name(CUID comp)
{
    const ComponentBase* base = mObj->scene.get_component_base(comp);
    if (!base)
        return nullptr;

    return base->name;
}

void EditorContext::create_component_script_slot(CUID compID, AUID assetID)
{
    mObj->scene.create_component_script_slot(compID, assetID);
}

const ComponentScriptSlot* EditorContext::get_component_script_slot(CUID compID)
{
    return mObj->scene.get_component_script_slot(compID);
}

void EditorContext::set_selected_component(CUID comp)
{
    if (mObj->selectedComponent == comp)
        return;

    // update state and notify observers
    EditorContextComponentSelectionEvent event(comp);
    mObj->selectedComponent = comp;
    mObj->selectedComponentRUID = mObj->scene.get_component_ruid(comp);
    mObj->notify_observers(&event);
}

CUID EditorContext::get_selected_component()
{
    return mObj->selectedComponent;
}

void* EditorContext::get_component(CUID compID, ComponentType& type)
{
    return mObj->scene.get_component(compID, type);
}

CUID EditorContext::get_ruid_component(RUID ruid)
{
    return mObj->scene.get_ruid_component(ruid);
}

void EditorContext::set_mesh_component_asset(CUID meshC, AUID meshAssetID)
{
    return mObj->scene.set_mesh_component_asset(meshC, meshAssetID);
}

RUID EditorContext::get_selected_component_ruid()
{
    return mObj->selectedComponentRUID;
}

bool EditorContext::get_selected_component_transform(Transform& transform)
{
    return mObj->scene.get_component_transform(mObj->selectedComponent, transform);
}

bool EditorContext::set_component_transform(CUID compID, const Transform& transform)
{
    return mObj->scene.set_component_transform(compID, transform);
}

bool EditorContext::get_component_transform_mat4(CUID compID, Mat4& worldMat4)
{
    return mObj->scene.get_component_transform_mat4(compID, worldMat4);
}

} // namespace LD