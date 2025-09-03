#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("EditorContext");

using EditorContextObserver = std::pair<EditorContextEventFn, void*>;

/// @brief Editor context implementation. Keeps track of the active project
///        and active scene states.
struct EditorContextObj
{
    RServer renderServer;             /// render server handle
    Project project;                  /// current project under edit
    Scene scene;                      /// current scene under edit
    AssetManager assetManager;        /// loads assets for the scene
    EditorSettings settings;          /// editor global settings
    fs::path sceneTOMLPath;           /// path to current scene file
    fs::path assetTOMLPath;           /// path to project asset file
    fs::path projectDirPath;          /// path to project root directory
    std::string projectName;          /// project identifier
    std::vector<fs::path> scenePaths; /// path to scene files in project
    std::vector<EditorContextObserver> observers;
    DUID selectedComponent;
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
    obj->settings = EditorSettings::create_default();
    obj->isPlaying = false;

    return {obj};
}

void EditorContext::destroy(EditorContext ctx)
{
    EditorContextObj* obj = ctx;

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

    return self.scene.get_ruid_transform(ruid);
}

EditorSettings EditorContext::get_settings()
{
    return mObj->settings;
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

    fs::path projectDirPath = filePath.parent_path();

    TOMLDocument projectDoc = TOMLDocument::create_from_file(filePath);
    mObj->project = Project::create(projectDirPath);
    ProjectSchema::load_project(mObj->project, projectDoc);
    TOMLDocument::destroy(projectDoc);

    mObj->projectDirPath = projectDirPath;
    mObj->projectName = mObj->project.get_name();

    fs::path assetsPath = mObj->project.get_assets_path();
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

    for (const fs::path& scenePath : mObj->scenePaths)
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

    // TODO:
    mObj->scene.startup();
}

void EditorContext::stop_scene()
{
    LD_PROFILE_SCOPE;

    if (!mObj->isPlaying)
        return;

    mObj->isPlaying = false;

    // TODO:
    mObj->scene.cleanup();
}

bool EditorContext::is_playing()
{
    return mObj->isPlaying;
}

void EditorContext::get_scene_roots(std::vector<DUID>& roots)
{
    mObj->scene.get_root_components(roots);
}

const ComponentBase* EditorContext::get_component_base(DUID comp)
{
    return mObj->scene.get_component_base(comp);
}

const char* EditorContext::get_component_name(DUID comp)
{
    const ComponentBase* base = mObj->scene.get_component_base(comp);
    if (!base)
        return nullptr;

    return base->name;
}

void EditorContext::set_selected_component(DUID comp)
{
    if (mObj->selectedComponent == comp)
        return;

    // update state and notify observers
    EditorContextComponentSelectionEvent event(comp);
    mObj->selectedComponent = comp;
    mObj->selectedComponentRUID = mObj->scene.get_component_ruid(comp);
    mObj->notify_observers(&event);
}

DUID EditorContext::get_selected_component()
{
    return mObj->selectedComponent;
}

void* EditorContext::get_component(DUID compID, ComponentType& type)
{
    return mObj->scene.get_component(compID, type);
}

DUID EditorContext::get_ruid_component(RUID ruid)
{
    return mObj->scene.get_ruid_component(ruid);
}

RUID EditorContext::get_selected_component_ruid()
{
    return mObj->selectedComponentRUID;
}

Transform* EditorContext::get_selected_component_transform()
{
    return mObj->scene.get_component_transform(mObj->selectedComponent);
}

} // namespace LD