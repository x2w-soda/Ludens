#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DSA/Observer.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Timer.h>
#include <LudensEditor/EditorContext/EditorAction.h>
#include <LudensEditor/EditorContext/EditorContext.h>

#include <utility>

#include "EditorContextCommand.h"

namespace LD {

static Log sLog("EditorContext");

/// @brief Editor context implementation. Keeps track of the active project
///        and active scene states.
struct EditorContextObj
{
    RenderSystem renderSystem;     /// render server handle
    AudioSystem audioSystem;       /// audio server handle
    Image2D iconAtlas;             /// editor icon atlas handle
    Project project;               /// current project under edit
    Scene scene;                   /// current scene under edit
    AssetManager assetManager;     /// loads assets for the scene
    EditorSettings settings;       /// editor global settings
    EditorActionQueue actionQueue; /// each action maps to one or more EditCommands.
    EditStack editStack;           /// undo/redo stack of EditCommands
    FS::Path iconAtlasPath;        /// path to editor icon atlas source file
    FS::Path sceneSchemaPath;      /// path to current scene file
    FS::Path assetSchemaPath;      /// path to project asset file
    FS::Path projectSchemaPath;    /// path to project file
    std::string projectName;       /// project identifier
    Vector<FS::Path> scenePaths;   /// path to scene schema files in project
    ObserverList<const EditorEvent*> observers;
    SUID selectedComponentSUID = 0;
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

    struct EditorActionOpenProjectParams
    {
        FS::Path schemaPath; // path to project schema
    } openProjectParams;

    struct EditorActionAddComponentParams
    {
        SUID parentSUID;
        ComponentType compType;
    } addComponentParams;

    struct EditorActionAddComponentScriptParams
    {
        SUID compSUID;         // component ID in current scene
        AssetID scriptAssetID; // script asset ID in project
    } addComponentScriptParams;

    struct EditorActionSetComponentAssetParams
    {
        SUID compSUID;
        AssetID assetID;
    } setComponentAssetParams;

    void notify_observers(const EditorEvent* event);

    void load_project(const FS::Path& projectSchemaPath);
    void load_project_scene(const FS::Path& sceneSchemaPath);
    void new_project_scene(const FS::Path& sceneSchemaPath);
    void save_scene_schema();
    void save_project_schema();
};

static void editor_action_undo(EditStack stack, void* user);
static void editor_action_redo(EditStack stack, void* user);
static void editor_action_new_scene(EditStack stack, void* user);
static void editor_action_open_scene(EditStack stack, void* user);
static void editor_action_save_scene(EditStack stack, void* user);
static void editor_action_open_project(EditStack stack, void* user);
static void editor_action_add_component_script(EditStack stack, void* user);
static void editor_action_set_component_asset(EditStack stack, void* user);

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
    obj->save_scene_schema();
}

static void editor_action_open_project(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // TODO: save scene and project dialog?
    // TODO: how much to unwind? AssetManager for sure, EditorContext invalidation?
    LD_UNREACHABLE;
}

static void editor_action_add_component(EditStack stack, void* user)
{
    EditorContextObj* obj = (EditorContextObj*)user;
    const auto& params = obj->addComponentParams;

    stack.execute(EditStack::new_command<AddComponentCommand>(
        obj->scene,
        params.parentSUID,
        params.compType));
}

static void editor_action_add_component_script(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;
    const auto& params = obj->addComponentScriptParams;

    stack.execute(EditStack::new_command<AddComponentScriptCommand>(
        obj->scene,
        params.compSUID,
        params.scriptAssetID));
}

static void editor_action_set_component_asset(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    auto* obj = (EditorContextObj*)user;
    const auto& params = obj->setComponentAssetParams;

    stack.execute(EditStack::new_command<SetComponentAssetCommand>(
        obj->scene,
        params.compSUID,
        params.assetID));
}

void EditorContextObj::notify_observers(const EditorEvent* event)
{
    observers.notify(event);
}

void EditorContextObj::load_project(const FS::Path& projectSchemaPath)
{
    LD_PROFILE_SCOPE;

    this->projectSchemaPath = projectSchemaPath;
    const FS::Path projectDirPath = projectSchemaPath.parent_path();

    std::string err;
    project = Project::create(projectDirPath);
    bool ok = ProjectSchema::load_project_from_file(project, projectSchemaPath, err);
    LD_ASSERT(ok); // TODO:

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
    assetManager.begin_load_batch();
    assetManager.load_all_assets();
    assetManager.end_load_batch();

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

    EditorNotifyProjectLoadEvent event{};
    notify_observers(&event);
}

void EditorContextObj::load_project_scene(const FS::Path& sceneSchemaPath)
{
    LD_PROFILE_SCOPE;

    if (!FS::exists(sceneSchemaPath))
        return;

    this->sceneSchemaPath = sceneSchemaPath;
    selectedComponentSUID = 0;

    if (scene)
    {
        scene.unload();
    }
    else
    {
        SceneInfo sceneI{};
        sceneI.assetManager = assetManager;
        sceneI.renderSystem = renderSystem;
        sceneI.audioSystem = audioSystem;
        scene = Scene::create(sceneI);
    }

    scene.load([&](Scene scene) -> bool {
        // load the scene
        std::string err;
        return SceneSchema::load_scene_from_file(scene, sceneSchemaPath, err);
    });

    // TODO: check scene load success

    EditorNotifySceneLoadEvent event{};
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

    sLog.info("created new scene {}", newSchemaPath.string());

    load_project_scene(newSchemaPath);

    // TODO: maybe notify observers?
}

void EditorContextObj::save_scene_schema()
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

void EditorContextObj::save_project_schema()
{
    if (!project || projectSchemaPath.empty())
        return;

    Timer timer;
    timer.start();

    std::string err;
    ProjectSchema::save_project(project, projectSchemaPath, err);

    size_t us = timer.stop();
    sLog.info("saved project to {} ({} ms)", projectSchemaPath.string(), us / 1000.0f);
}

EditorContext EditorContext::create(const EditorContextInfo& info)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = heap_new<EditorContextObj>(MEMORY_USAGE_MISC);
    obj->renderSystem = info.renderSystem;
    obj->audioSystem = info.audioSystem;
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
        {EDITOR_ACTION_OPEN_PROJECT,         &editor_action_open_project,         "OpenProject"},
        {EDITOR_ACTION_ADD_COMPONENT,        &editor_action_add_component,        "AddComponent"},
        {EDITOR_ACTION_ADD_COMPONENT_SCRIPT, &editor_action_add_component_script, "AddComponentScript"},
        {EDITOR_ACTION_SET_COMPONENT_ASSET,  &editor_action_set_component_asset,  "SetComponentAsset"},
    };
    // clang-format on

    for (size_t i = 0; i < sizeof(editorActions) / sizeof(*editorActions); i++)
        EditorAction::register_action(editorActions[i]);

    return {obj};
}

void EditorContext::destroy(EditorContext ctx)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = ctx;

    if (obj->iconAtlas)
    {
        obj->renderSystem.destroy_image_2d(obj->iconAtlas);
        obj->iconAtlas = {};
    }

    Project::destroy(obj->project);
    Scene::destroy(obj->scene);

    if (obj->assetManager)
    {
        AssetManager::destroy(obj->assetManager);
        obj->assetManager = {};
    }

    EditorActionQueue::destroy(obj->actionQueue);
    EditStack::destroy(obj->editStack);
    EditorSettings::destroy(obj->settings);

    heap_delete<EditorContextObj>(obj);
}

bool EditorContext::render_system_mat4_callback(RUID ruid, Mat4& mat4, void* user)
{
    EditorContextObj& self = *(EditorContextObj*)user;

    return self.scene.get_ruid_world_mat4(ruid, mat4);
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

void EditorContext::action_open_project(const FS::Path& projectSchemaPath)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_OPEN_PROJECT);
    mObj->openProjectParams.schemaPath = projectSchemaPath;
}

void EditorContext::action_save_scene()
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_SAVE_SCENE);
}

void EditorContext::action_add_component(SUID parentSUID, ComponentType type)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT);
    mObj->addComponentParams.parentSUID = parentSUID;
    mObj->addComponentParams.compType = type;
}

void EditorContext::action_add_component_script(SUID compSUID, AssetID scriptAssetID)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT_SCRIPT);
    mObj->addComponentScriptParams.compSUID = compSUID;
    mObj->addComponentScriptParams.scriptAssetID = scriptAssetID;
}

void EditorContext::action_set_component_asset(SUID compSUID, AssetID assetID)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_SET_COMPONENT_ASSET);
    mObj->setComponentAssetParams.compSUID = compSUID;
    mObj->setComponentAssetParams.assetID = assetID;
}

void EditorContext::poll_actions()
{
    mObj->actionQueue.poll_actions();
}

FS::Path EditorContext::get_project_directory()
{
    return mObj->projectSchemaPath.parent_path();
}

FS::Path EditorContext::get_scene_schema_path()
{
    return mObj->sceneSchemaPath;
}

EditorSettings EditorContext::get_settings()
{
    return mObj->settings;
}

ProjectSettings EditorContext::get_project_settings()
{
    if (!mObj->project)
        return {};

    return mObj->project.get_settings();
}

AssetManager EditorContext::get_asset_manager()
{
    return mObj->assetManager;
}

RImage EditorContext::get_editor_icon_atlas()
{
    if (!mObj->iconAtlas)
    {
        std::string iconAtlasPath = mObj->iconAtlasPath.string();
        Bitmap tmpBitmap = Bitmap::create_from_path(iconAtlasPath.c_str(), false);
        mObj->iconAtlas = mObj->renderSystem.create_image_2d(tmpBitmap);
        Bitmap::destroy(tmpBitmap);
    }

    LD_ASSERT(mObj->iconAtlas);
    return RImage(mObj->iconAtlas.unwrap());
}

Scene EditorContext::get_scene()
{
    return mObj->scene;
}

Camera EditorContext::get_scene_camera()
{
    return mObj->scene.get_camera();
}

void EditorContext::add_observer(EditorEventFn fn, void* user)
{
    mObj->observers.add_observer(fn, user);
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
}

bool EditorContext::is_playing()
{
    return mObj->isPlaying;
}

void EditorContext::get_scene_roots(Vector<Scene::Component>& roots)
{
    return mObj->scene.get_root_components(roots);
}

const char* EditorContext::get_component_name(SUID compSUID)
{
    Scene::Component comp = mObj->scene.get_component_by_suid(compSUID);
    if (!comp)
        return nullptr;

    return comp.get_name();
}

void EditorContext::request_event(const EditorRequestEvent* event)
{
    mObj->notify_observers(event);
}

void EditorContext::set_selected_component(SUID compSUID)
{
    if (mObj->selectedComponentSUID == compSUID)
        return;

    Scene::Component comp = mObj->scene.get_component_by_suid(compSUID);
    if (comp)
    {
        // update state and notify observers
        EditorNotifyComponentSelectionEvent event(compSUID);
        mObj->selectedComponentSUID = compSUID;
        mObj->notify_observers(&event);
    }
    else
    {
        mObj->selectedComponentSUID = 0;
    }
}

SUID EditorContext::get_selected_component()
{
    return mObj->selectedComponentSUID;
}

Scene::Component EditorContext::get_component(SUID compSUID)
{
    return mObj->scene.get_component_by_suid(compSUID);
}

Scene::Component EditorContext::get_component_by_ruid(RUID ruid)
{
    return mObj->scene.get_ruid_component(ruid);
}

RUID EditorContext::get_selected_component_ruid()
{
    Scene::Component comp = mObj->scene.get_component_by_suid(mObj->selectedComponentSUID);

    return comp ? comp.ruid() : 0;
}

bool EditorContext::get_selected_component_transform(TransformEx& transform)
{
    Scene::Component comp = mObj->scene.get_component_by_suid(mObj->selectedComponentSUID);

    if (!comp)
        return false;

    return comp.get_transform(transform);
}

UILayoutInfo EditorContext::make_vbox_layout()
{
    return mObj->settings.get_theme().make_vbox_layout();
}

UILayoutInfo EditorContext::make_hbox_layout()
{
    return mObj->settings.get_theme().make_hbox_layout();
}

} // namespace LD