#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DSA/Observer.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataComponent.h>
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
    RenderServer renderServer;     /// render server handle
    AudioServer audioServer;       /// audio server handle
    RImage iconAtlas;              /// editor icon atlas handle
    Project project;               /// current project under edit
    Scene scene;                   /// current scene under edit
    AssetManager assetManager;     /// loads assets for the scene
    EditorSettings settings;       /// editor global settings
    EditorActionQueue actionQueue; /// each action maps to one or more EditCommands.
    EditStack editStack;           /// undo/redo stack of EditCommands
    FS::Path iconAtlasPath;        /// path to editor icon atlas source file
    FS::Path sceneSchemaPath;      /// path to current scene file
    FS::Path assetSchemaPath;      /// path to project asset file
    FS::Path projectDirPath;       /// path to project root directory
    std::string projectName;       /// project identifier
    Vector<FS::Path> scenePaths;   /// path to scene schema files in project
    ObserverList<const EditorEvent*> observers;
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

    struct EditorActionAddComponentParams
    {
        CUID parentID;
        ComponentType compType;
    } addComponentParams;

    struct EditorActionAddComponentScriptParams
    {
        CUID compID;        // component ID in current scene
        AUID scriptAssetID; // script asset ID in project
    } addComponentScriptParams;

    struct EditorActionSetComponentAssetParams
    {
        CUID compID;
        AUID assetID;
    } setComponentAssetParams;

    void notify_observers(const EditorEvent* event);

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
    obj->save_project_scene();
}

static void editor_action_add_component(EditStack stack, void* user)
{
    EditorContextObj* obj = (EditorContextObj*)user;
    const auto& params = obj->addComponentParams;

    stack.execute(EditStack::new_command<AddComponentCommand>(
        obj->scene,
        params.parentID,
        params.compType));
}

static void editor_action_add_component_script(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;
    const auto& params = obj->addComponentScriptParams;

    stack.execute(EditStack::new_command<AddComponentScriptCommand>(
        obj->scene,
        params.compID,
        params.scriptAssetID));
}

static void editor_action_set_component_asset(EditStack stack, void* user)
{
    LD_PROFILE_SCOPE;

    auto* obj = (EditorContextObj*)user;
    const auto& params = obj->setComponentAssetParams;

    stack.execute(EditStack::new_command<SetComponentAssetCommand>(
        obj->scene,
        params.compID,
        params.assetID));
}

void EditorContextObj::notify_observers(const EditorEvent* event)
{
    observers.notify(event);
}

void EditorContextObj::load_project(const FS::Path& projectSchemaPath)
{
    LD_PROFILE_SCOPE;

    projectDirPath = projectSchemaPath.parent_path();

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
    selectedComponent = 0;
    selectedComponentRUID = 0;

    SceneInfo sceneI{};
    sceneI.assetManager = assetManager;
    sceneI.renderServer = renderServer;
    sceneI.audioServer = audioServer;
    scene = Scene::create(sceneI);

    // load the scene
    std::string err;
    bool ok = SceneSchema::load_scene_from_file(scene, sceneSchemaPath, err);
    LD_ASSERT(ok); // TODO:
    scene.load();

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
    LD_PROFILE_SCOPE;

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
        RDevice device = obj->renderServer.get_device();
        device.wait_idle();
        device.destroy_image(obj->iconAtlas);
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

Mat4 EditorContext::render_server_transform_callback(RUID ruid, void* user)
{
    EditorContextObj& self = *(EditorContextObj*)user;

    return self.scene.get_ruid_transform_mat4(ruid);
}

ScreenLayer EditorContext::render_server_screen_pass_callback(void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj& self = *(EditorContextObj*)user;

    // SPACE: In the editor maybe we can filter what screen layers to render?

    return self.scene.get_screen_layer();
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

void EditorContext::action_add_component(CUID parentID, ComponentType type)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT);
    mObj->addComponentParams.parentID = parentID;
    mObj->addComponentParams.compType = type;
}

void EditorContext::action_add_component_script(CUID compID, AUID scriptAssetID)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT_SCRIPT);
    mObj->addComponentScriptParams.compID = compID;
    mObj->addComponentScriptParams.scriptAssetID = scriptAssetID;
}

void EditorContext::action_set_component_asset(CUID compID, AUID assetID)
{
    mObj->actionQueue.enqueue(EDITOR_ACTION_SET_COMPONENT_ASSET);
    mObj->setComponentAssetParams.compID = compID;
    mObj->setComponentAssetParams.assetID = assetID;
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

void EditorContext::get_scene_roots(Vector<CUID>& roots)
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

void EditorContext::request_event(const EditorRequestEvent* event)
{
    mObj->notify_observers(event);
}

void EditorContext::set_selected_component(CUID comp)
{
    if (mObj->selectedComponent == comp)
        return;

    // update state and notify observers
    EditorNotifyComponentSelectionEvent event(comp);
    mObj->selectedComponent = comp;
    mObj->selectedComponentRUID = mObj->scene.get_component_ruid(comp);
    mObj->notify_observers(&event);
}

CUID EditorContext::get_selected_component()
{
    return mObj->selectedComponent;
}

void* EditorContext::get_component(CUID compID, ComponentType* outType)
{
    return mObj->scene.get_component(compID, outType);
}

CUID EditorContext::get_ruid_component(RUID ruid)
{
    return mObj->scene.get_ruid_component(ruid);
}

RUID EditorContext::get_selected_component_ruid()
{
    return mObj->selectedComponentRUID;
}

bool EditorContext::get_selected_component_transform(TransformEx& transform)
{
    return mObj->scene.get_component_transform(mObj->selectedComponent, transform);
}

bool EditorContext::set_component_transform(CUID compID, const TransformEx& transform)
{
    return mObj->scene.set_component_transform(compID, transform);
}

bool EditorContext::get_component_transform_mat4(CUID compID, Mat4& worldMat4)
{
    return mObj->scene.get_component_transform_mat4(compID, worldMat4);
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