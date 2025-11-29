#include "EditorContextCommand.h"
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Assert.h>
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
#include <Ludens/System/Timer.h>
#include <LudensEditor/EditorContext/EditorAction.h>
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
    AudioServer audioServer;          /// audio server handle
    RImage iconAtlas;                 /// editor icon atlas handle
    Project project;                  /// current project under edit
    Scene scene;                      /// current scene under edit
    AssetManager assetManager;        /// loads assets for the scene
    EditorSettings settings;          /// editor global settings
    EditorActionQueue actionQueue;    /// each action maps to one or more EditCommands.
    EditStack editStack;              /// undo/redo stack of EditCommands
    FS::Path iconAtlasPath;           /// path to editor icon atlas source file
    FS::Path sceneSchemaPath;         /// path to current scene file
    FS::Path assetSchemaPath;         /// path to project asset file
    FS::Path projectDirPath;          /// path to project root directory
    std::string projectName;          /// project identifier
    std::vector<FS::Path> scenePaths; /// path to scene schema files in project
    std::vector<EditorContextObserver> observers;
    CUID selectedComponent;
    RUID selectedComponentRUID;
    bool isPlaying;

    // TODO: union of all params? or can we accumulate params for multiple actions simultaneously?
    struct EditorActionNewSceneParams
    {
        FS::Path schemaPath; // path to save schema for new scene
    } newSceneParams;

    struct EditorActionOpenSceneParams
    {
        FS::Path schemaPath; // path to scene schema
    } openSceneParams;

    struct EditorActionAddComponentScriptParams
    {
        CUID compID;        // component ID in current scene
        AUID scriptAssetID; // script asset ID in project
    } addComponentScriptParams;

    void notify_observers(const EditorContextEvent* event);

    void load_project(const FS::Path& projectSchemaPath);
    void load_project_scene(const FS::Path& sceneSchemaPath);
    void new_project_scene(const FS::Path& sceneSchemaPath);
    void save_project_scene();
};

static void editor_action_undo(EditStack stack, void* user);
static void editor_action_redo(EditStack stack, void* user);
static void editor_action_new_scene(EditStack stack, void* user);
static void editor_action_open_scene(EditStack stack, void* user);
static void editor_action_save_scene(EditStack stack, void* user);
static void editor_action_add_component_script(EditStack stack, void* user);

static void editor_action_undo(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    stack.undo();
}

static void editor_action_redo(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    stack.redo();
}

static void editor_action_new_scene(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // Creating new Scene would clear the EditStack
    stack.clear();

    obj->new_project_scene(obj->newSceneParams.schemaPath);
}

static void editor_action_open_scene(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // Opening Scene would clear the EditStack
    stack.clear();

    obj->load_project_scene(obj->openSceneParams.schemaPath);
}

static void editor_action_save_scene(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // Saving Scene writes the current schema to disk and
    // should not effect the EditStack.
    obj->save_project_scene();
}

static void editor_action_add_component_script(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;
    EditorContext ctx(obj);

    const auto& params = obj->addComponentScriptParams;

    stack.execute(EditStack::new_command<AddComponentScriptCommand>(
        obj->scene,
        params.compID,
        params.scriptAssetID));
}

void EditorContextObj::notify_observers(const EditorContextEvent* event)
{
    for (auto& observer : observers)
    {
        observer.first(event, observer.second);
    }
}

void EditorContextObj::load_project(const FS::Path& projectSchemaPath)
{
    LD_PROFILE_SCOPE;

    projectDirPath = projectSchemaPath.parent_path();

    project = Project::create(projectDirPath);
    ProjectSchema::load_project_from_file(project, projectSchemaPath);

    projectName = project.get_name();

    FS::Path assetsPath = project.get_assets_path();
    sLog.info("loading project [{}], root directory {}", projectName, projectDirPath.string());

    assetSchemaPath = assetsPath;

    if (!FS::exists(assetSchemaPath))
    {
        sLog.warn("failed to find project assets {}", assetSchemaPath.string());
        return;
    }

    if (assetManager)
    {
        AssetManager::destroy(assetManager);
    }

    AssetManagerInfo amI{};
    amI.rootPath = projectDirPath;
    amI.watchAssets = true;
    amI.assetSchemaPath = assetSchemaPath;
    assetManager = AssetManager::create(amI);

    // Load all project assets at once using job system.
    // Once we have asynchronous-load-jobs maybe we can load assets
    // used by the loaded scene first?
    assetManager.load_all_assets();

    project.get_scene_paths(scenePaths);

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
        load_project_scene(scenePaths.front());

    EditorContextProjectLoadEvent event{};
    notify_observers(&event);
}

void EditorContextObj::load_project_scene(const FS::Path& sceneSchemaPath)
{
    LD_PROFILE_SCOPE;

    if (!FS::exists(sceneSchemaPath))
        return;

    this->sceneSchemaPath = sceneSchemaPath;
    selectedComponent = 0;
    selectedComponentRUID = 0;

    if (scene)
        Scene::destroy(scene);

    // create the scene from schema
    scene = Scene::create();
    SceneSchema::load_scene_from_file(scene, sceneSchemaPath);

    // load the scene
    SceneLoadInfo loadInfo{};
    loadInfo.assetManager = assetManager;
    loadInfo.renderServer = renderServer;
    loadInfo.audioServer = audioServer;
    scene.load(loadInfo);

    EditorContextSceneLoadEvent event{};
    notify_observers(&event);
}

void EditorContextObj::new_project_scene(const FS::Path& newSchemaPath)
{
    EditorContext ctx(this);

    if (newSchemaPath.empty())
        return;

    if (FS::exists(newSchemaPath))
    {
        sLog.warn("new_project_scene failure: scene already exists {}", newSchemaPath.string());
        return;
    }

    std::string newSchemaText = SceneSchema::get_default_text();

    if (!FS::write_file(newSchemaPath, (uint64_t)newSchemaText.size(), (const byte*)newSchemaText.data()))
    {
        sLog.warn("new_project_scene failure: failed to write to {}", newSchemaPath.string());
        return;
    }

    sLog.info("created new scene {}", newSchemaPath.string());

    load_project_scene(newSchemaPath);

    // TODO: maybe notify observers?
}

void EditorContextObj::save_project_scene()
{
    if (!scene || sceneSchemaPath.empty())
        return;

    Timer timer;
    timer.start();

    std::string err;
    SceneSchema::save_scene(scene, sceneSchemaPath, err);

    size_t us = timer.stop();
    sLog.info("saved scene to {} ({} ms)", sceneSchemaPath.string(), us / 1000.0f);
}

EditorContext EditorContext::create(const EditorContextInfo& info)
{
    EditorContextObj* obj = heap_new<EditorContextObj>(MEMORY_USAGE_MISC);
    obj->renderServer = info.renderServer;
    obj->audioServer = info.audioServer;
    obj->iconAtlasPath = info.iconAtlasPath;
    obj->settings = EditorSettings::create_default();
    obj->isPlaying = false;
    obj->editStack = EditStack::create();
    obj->actionQueue = EditorActionQueue::create(obj->editStack, obj);

    // register possible editor actions
    // clang-format off
    const EditorActionInfo editorActions[] = {
        {EDITOR_ACTION_UNDO,                 &editor_action_undo,                 "Undo"},
        {EDITOR_ACTION_REDO,                 &editor_action_redo,                 "Redo"},
        {EDITOR_ACTION_NEW_SCENE,            &editor_action_new_scene,            "NewScene"}, 
        {EDITOR_ACTION_OPEN_SCENE,           &editor_action_open_scene,           "OpenScene"}, 
        {EDITOR_ACTION_SAVE_SCENE,           &editor_action_save_scene,           "SaveScene"},
        {EDITOR_ACTION_ADD_COMPONENT_SCRIPT, &editor_action_add_component_script, "AddComponentScript"},
    };
    // clang-format on

    for (size_t i = 0; i < sizeof(editorActions) / sizeof(*editorActions); i++)
        EditorAction::register_action(editorActions[i]);

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
    EditorActionQueue::destroy(obj->actionQueue);
    EditStack::destroy(obj->editStack);
    EditorSettings::destroy(obj->settings);

    heap_delete<EditorContextObj>(obj);
}

Mat4 EditorContext::render_server_transform_callback(RUID ruid, void* user)
{
    EditorContextObj& self = *(EditorContextObj*)user;

    return self.scene.get_ruid_transform_mat4(ruid);
}

void EditorContext::action_redo()
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_REDO);
}

void EditorContext::action_undo()
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_UNDO);
}

void EditorContext::action_new_scene(const FS::Path& sceneSchemaPath)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_NEW_SCENE);
    mObj->newSceneParams.schemaPath = sceneSchemaPath;
}

void EditorContext::action_open_scene(const FS::Path& sceneSchemaPath)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_OPEN_SCENE);
    mObj->openSceneParams.schemaPath = sceneSchemaPath;
}

void EditorContext::action_save_scene()
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_SAVE_SCENE);
}

void EditorContext::action_add_component_script(CUID compID, AUID scriptAssetID)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT_SCRIPT);
    mObj->addComponentScriptParams.compID = compID;
    mObj->addComponentScriptParams.scriptAssetID = scriptAssetID;
}

void EditorContext::poll_actions()
{
    mObj->actionQueue.poll_actions();
}

FS::Path EditorContext::get_project_directory()
{
    return mObj->projectDirPath;
}

FS::Path EditorContext::get_scene_schema_path()
{
    return mObj->sceneSchemaPath;
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

Scene EditorContext::get_scene()
{
    return mObj->scene;
}

Camera EditorContext::get_scene_camera()
{
    return mObj->scene.get_camera();
}

void EditorContext::add_observer(EditorContextEventFn fn, void* user)
{
    mObj->observers.push_back(std::make_pair(fn, user));
}

void EditorContext::update(const Vec2& sceneExtent, float delta)
{
    if (mObj->isPlaying)
    {
        mObj->scene.update(sceneExtent, delta);
    }

    // NOTE: this polls for any asset file changes.
    mObj->assetManager.update();
}

void EditorContext::load_project(const FS::Path& projectSchemaPath)
{
    mObj->load_project(projectSchemaPath);
}

void EditorContext::load_project_scene(const FS::Path& sceneSchemaPath)
{
    mObj->load_project_scene(sceneSchemaPath);
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