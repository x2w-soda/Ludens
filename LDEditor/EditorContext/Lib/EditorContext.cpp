#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
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
    Scene scene;                      /// subject scene under edit.
    AssetManager assetManager;        /// loads assets for the scene
    EditorSettings settings;          /// editor global settings
    fs::path sceneJSONPath;           /// path to current scene file
    fs::path assetJSONPath;           /// path to project asset file
    fs::path projectDirPath;          /// path to project root directory
    std::string projectName;          /// project identifier
    std::vector<fs::path> scenePaths; /// path to scene files in project
    std::vector<EditorContextObserver> observers;
    DUID selectedComponent;
    RUID selectedComponentRUID;
    JSONDocument projectDoc;
    JSONDocument assetDoc;
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

    if (obj->assetDoc)
    {
        JSONDocument::destroy(obj->assetDoc);
        obj->assetDoc = {};
    }

    if (obj->projectDoc)
    {
        JSONDocument::destroy(obj->projectDoc);
        obj->projectDoc = {};
    }

    if (obj->assetManager)
    {
        AssetManager::destroy(obj->assetManager);
        obj->assetManager = {};
    }

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

    mObj->projectDoc = JSONDocument::create_from_file(filePath);
    JSONNode root;

    if (!mObj->projectDoc || !(root = mObj->projectDoc.get_root()))
    {
        sLog.warn("failed to load project {}", filePath.string());
        return;
    }

    mObj->projectDirPath = filePath.parent_path();

    JSONNode versionNode = root.get_member("version");
    uint32_t version;
    if (!versionNode || !versionNode.is_u32(&version))
        return;

    JSONNode nameNode = root.get_member("name");
    if (!nameNode || !nameNode.is_string(&mObj->projectName))
        return;

    JSONNode assetFileNode = root.get_member("assets");
    std::string assetFile;
    if (!assetFileNode || !assetFileNode.is_string(&assetFile))
        return;

    sLog.info("loading project [{}], root directory {}", mObj->projectName, mObj->projectDirPath.string());

    mObj->assetJSONPath = mObj->projectDirPath / fs::path(assetFile);

    if (!FS::exists(mObj->assetJSONPath))
    {
        sLog.warn("failed to find project assets {}", mObj->assetJSONPath.string());
        return;
    }

    if (mObj->assetManager)
    {
        AssetManager::destroy(mObj->assetManager);
    }

    // Load all project assets at once using job system.
    // Once we have asynchronous-load-jobs maybe we can load assets
    // used by the loaded scene first?
    mObj->assetDoc = JSONDocument::create_from_file(mObj->assetJSONPath);
    mObj->assetManager = AssetManager::create(mObj->projectDirPath);
    mObj->assetManager.load_assets(mObj->assetDoc);

    JSONNode scenesNode = root.get_member("scenes");
    if (!scenesNode || !scenesNode.is_array())
        return;

    mObj->scenePaths.clear();
    int sceneCount = scenesNode.get_size();

    for (int i = 0; i < sceneCount; i++)
    {
        JSONNode sceneFileNode = scenesNode[i];
        std::string sceneFileString;
        if (!sceneFileNode || !sceneFileNode.is_string(&sceneFileString))
            continue;

        fs::path sceneFilePath = mObj->projectDirPath / fs::path(sceneFileString);
        if (!FS::exists(sceneFilePath))
            continue;

        mObj->scenePaths.push_back(sceneFilePath);
        sLog.info("- found scene {}", sceneFilePath.string());
    }

    if (!mObj->scenePaths.empty())
        load_project_scene(mObj->scenePaths.front());

    EditorContextProjectLoadEvent event{};
    mObj->notify_observers(&event);
}

void EditorContext::load_project_scene(const std::filesystem::path& jsonPath)
{
    LD_PROFILE_SCOPE;

    mObj->sceneJSONPath = jsonPath;
    mObj->selectedComponent = 0;
    mObj->selectedComponentRUID = 0;

    SceneInfo sceneI;
    sceneI.jsonDoc = JSONDocument::create_from_file(mObj->sceneJSONPath);
    sceneI.assetManager = mObj->assetManager;
    sceneI.renderServer = mObj->renderServer;
    mObj->scene = Scene::create(sceneI);
    JSONDocument::destroy(sceneI.jsonDoc);

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

const char* EditorContext::get_component_name(DUID comp)
{
    const DataComponent* base = mObj->scene.get_component_base(comp);
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