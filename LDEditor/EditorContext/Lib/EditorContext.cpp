#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DSA/HashMap.h>
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
#include <Ludens/Scene/Component/Sprite2DView.h>
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
    FontAtlas defaultFontAtlas;    ///
    FontAtlas monoFontAtlas;       ///
    RImage defaultFontAtlasImage;  ///
    RImage monoFontAtlasImage{};   ///
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
    HashMap<KeyValue, EditorActionType> keyBinds;
    ObserverList<const EditorEvent*> observers;
    CUID selectedComponentCUID = 0;
    CUID prevSelectedComponentCUID = 0;
    bool isPlaying;

    void notify_observers(const EditorEvent* event);

    void load_project(const FS::Path& projectSchemaPath);
    void load_project_scene(const FS::Path& sceneSchemaPath);
    void new_project_scene(const FS::Path& sceneSchemaPath);
    void save_scene_schema();
    void save_project_schema();
};

static void editor_action_undo(EditStack stack, const EditorAction& action, void* user);
static void editor_action_redo(EditStack stack, const EditorAction& action, void* user);
static void editor_action_new_scene(EditStack stack, const EditorAction& action, void* user);
static void editor_action_open_scene(EditStack stack, const EditorAction& action, void* user);
static void editor_action_save_scene(EditStack stack, const EditorAction& action, void* user);
static void editor_action_open_project(EditStack stack, const EditorAction& action, void* user);
static void editor_action_add_component(EditStack stack, const EditorAction& action, void* user);
static void editor_action_add_component_script(EditStack stack, const EditorAction& action, void* user);
static void editor_action_set_component_asset(EditStack stack, const EditorAction& action, void* user);
static void editor_action_clone_component_subtree(EditStack stack, const EditorAction& action, void* user);
static void editor_action_delete_component_subtree(EditStack stack, const EditorAction& action, void* user);
static void editor_action_close_dialog(EditStack stack, const EditorAction& action, void* user);

static void editor_action_undo(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    stack.undo();
}

static void editor_action_redo(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    stack.redo();
}

static void editor_action_new_scene(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // Creating new Scene would clear the EditStack
    stack.clear();

    obj->new_project_scene(action.newScene);
}

static void editor_action_open_scene(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // Opening Scene would clear the EditStack
    stack.clear();

    obj->load_project_scene(action.openScene);
}

static void editor_action_save_scene(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // Saving Scene writes the current schema to disk and
    // should not effect the EditStack.
    obj->save_scene_schema();
}

static void editor_action_open_project(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    // TODO: save scene and project dialog?
    // TODO: how much to unwind? AssetManager for sure, EditorContext invalidation?
    LD_UNREACHABLE;
}

static void editor_action_add_component(EditStack stack, const EditorAction& action, void* user)
{
    EditorContextObj* obj = (EditorContextObj*)user;

    stack.execute(EditStack::new_command<AddComponentCommand>(
        obj->scene,
        action.addComponent.parentSUID,
        action.addComponent.compType));
}

static void editor_action_add_component_script(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = (EditorContextObj*)user;

    stack.execute(EditStack::new_command<AddComponentScriptCommand>(
        obj->scene,
        action.addComponentScript.compSUID,
        action.addComponentScript.assetID));
}

static void editor_action_set_component_asset(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    auto* obj = (EditorContextObj*)user;

    stack.execute(EditStack::new_command<SetComponentAssetCommand>(
        obj->scene,
        action.setComponentAsset.compSUID,
        action.setComponentAsset.assetID));
}

static void editor_action_clone_component_subtree(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    auto* obj = (EditorContextObj*)user;

    stack.execute(EditStack::new_command<CloneComponentSubtreeCommand>(
        obj,
        action.cloneComponentSubtree));
}

static void editor_action_delete_component_subtree(EditStack stack, const EditorAction& action, void* user)
{
    LD_PROFILE_SCOPE;

    auto* obj = (EditorContextObj*)user;

    stack.execute(EditStack::new_command<DeleteComponentSubtreeCommand>(
        obj,
        action.deleteComponentSubtree));
}

static void editor_action_close_dialog(EditStack stack, const EditorAction& action, void* user)
{
    auto* obj = (EditorContextObj*)user;

    EditorRequestCloseDialogEvent event{};
    EditorContext(obj).request_event(&event);
}

void EditorContextObj::notify_observers(const EditorEvent* event)
{
    observers.notify(event);
}

void EditorContextObj::load_project(const FS::Path& projectSchemaPath)
{
    LD_PROFILE_SCOPE;

    this->projectSchemaPath = projectSchemaPath;

    std::string err;
    project = Project::create();
    bool ok = ProjectSchema::load_project_from_file(project, projectSchemaPath, err);
    LD_ASSERT(ok); // TODO:

    projectName = project.get_name();

    const FS::Path rootPath = project.get_root_path();
    sLog.info("loading project [{}], root directory {}", projectName, rootPath.string());

    assetSchemaPath = project.get_asset_schema_absolute_path();
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
    amI.rootPath = rootPath;
    amI.watchAssets = true;
    amI.assetSchemaPath = assetSchemaPath;
    assetManager = AssetManager::create(amI);

    // Load all project assets at once using job system.
    // Once we have asynchronous-load-jobs maybe we can load assets
    // used by the loaded scene first?
    assetManager.begin_load_batch();
    assetManager.load_all_assets();
    assetManager.end_load_batch();

    project.get_scene_absolute_paths(scenePaths);

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
    selectedComponentCUID = 0;

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
        sceneI.fontAtlas = defaultFontAtlas;
        sceneI.fontAtlasImage = defaultFontAtlasImage;
        sceneI.uiTheme = settings.get_theme().get_ui_theme();
        scene = Scene::create(sceneI);
    }

    scene.load([&](SceneObj* sceneObj) -> bool {
        // load the scene
        std::string err;
        return SceneSchema::load_scene_from_file(Scene(sceneObj), sceneSchemaPath, err);
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
    obj->defaultFontAtlas = info.defaultFontAtlas;
    obj->defaultFontAtlasImage = info.defaultFontAtlasImage;
    obj->monoFontAtlas = info.monoFontAtlas;
    obj->monoFontAtlasImage = info.monoFontAtlasImage;
    obj->settings = EditorSettings::create();
    obj->isPlaying = false;
    obj->editStack = EditStack::create();
    obj->actionQueue = EditorActionQueue::create(obj->editStack, obj);

    // register possible editor actions
    // clang-format off
    const EditorActionInfo editorActions[] = {
        {EDITOR_ACTION_UNDO,                     &editor_action_undo,                     "Undo"},
        {EDITOR_ACTION_REDO,                     &editor_action_redo,                     "Redo"},
        {EDITOR_ACTION_NEW_SCENE,                &editor_action_new_scene,                "NewScene"}, 
        {EDITOR_ACTION_OPEN_SCENE,               &editor_action_open_scene,               "OpenScene"}, 
        {EDITOR_ACTION_SAVE_SCENE,               &editor_action_save_scene,               "SaveScene"},
        {EDITOR_ACTION_OPEN_PROJECT,             &editor_action_open_project,             "OpenProject"},
        {EDITOR_ACTION_ADD_COMPONENT,            &editor_action_add_component,            "AddComponent"},
        {EDITOR_ACTION_ADD_COMPONENT_SCRIPT,     &editor_action_add_component_script,     "AddComponentScript"},
        {EDITOR_ACTION_SET_COMPONENT_ASSET,      &editor_action_set_component_asset,      "SetComponentAsset"},
        {EDITOR_ACTION_CLONE_COMPONENT_SUBTREE,  &editor_action_clone_component_subtree,  "CloneComponentSubtree"},
        {EDITOR_ACTION_DELETE_COMPONENT_SUBTREE, &editor_action_delete_component_subtree, "DeleteComponentSubtree"},
        {EDITOR_ACTION_CLOSE_DIALOG,             &editor_action_close_dialog,             "CloseDialog"},
    };
    // clang-format on

    for (size_t i = 0; i < sizeof(editorActions) / sizeof(*editorActions); i++)
        register_editor_action(editorActions[i]);

    // register default global key binds
    obj->keyBinds[KeyValue(KEY_CODE_Z, KEY_MOD_CONTROL_BIT)] = EDITOR_ACTION_UNDO;
    obj->keyBinds[KeyValue(KEY_CODE_R, KEY_MOD_CONTROL_BIT)] = EDITOR_ACTION_REDO;
    obj->keyBinds[KeyValue(KEY_CODE_S, KEY_MOD_CONTROL_BIT)] = EDITOR_ACTION_SAVE_SCENE;
    obj->keyBinds[KeyValue(KEY_CODE_A, KEY_MOD_CONTROL_BIT)] = EDITOR_ACTION_ADD_COMPONENT;
    obj->keyBinds[KeyValue(KEY_CODE_D, KEY_MOD_CONTROL_BIT)] = EDITOR_ACTION_CLONE_COMPONENT_SUBTREE;
    obj->keyBinds[KeyValue(KEY_CODE_DELETE)] = EDITOR_ACTION_DELETE_COMPONENT_SUBTREE;
    obj->keyBinds[KeyValue(KEY_CODE_ESCAPE)] = EDITOR_ACTION_CLOSE_DIALOG;

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

void EditorContext::render_system_screen_pass_overlay_callback(ScreenRenderComponent renderer, TView<int> regionVPIndices, int overlayVPIndex, void* user)
{
    EditorContextObj& self = *(EditorContextObj*)user;

    // TODO: Editor toggle to display screen UI?
    renderer.bind_quad_pipeline(QUAD_PIPELINE_UBER);
    renderer.set_view_projection_index(overlayVPIndex);
    self.scene.render_screen_ui(renderer);

    if (self.isPlaying)
        return;

    Mat4 worldMat4;
    ComponentView selectedComp = self.scene.get_component(self.selectedComponentCUID);
    if (selectedComp && selectedComp.get_world_mat4(worldMat4))
    {
        EditorTheme edTheme = self.settings.get_theme();
        Color hightlightColor;
        edTheme.get_gizmo_highlight_color(hightlightColor);

        if (selectedComp.type() == COMPONENT_TYPE_SPRITE_2D)
        {
            float thickness = 2.0f; // TODO: a function of Camera zoom?
            Sprite2DView sprite2D(selectedComp);
            Rect rect = Rect::grow(sprite2D.local_rect(), thickness);

            for (int i = 0; i < regionVPIndices.size; i++)
            {
                renderer.set_view_projection_index(regionVPIndices.data[i]);
                renderer.draw_rect_outline(worldMat4, rect, hightlightColor, thickness);
            }
        }
    }
}

void EditorContext::action(EditorActionType type)
{
    ComponentView selectedComp = mObj->scene.get_component(mObj->selectedComponentCUID);

    switch (type)
    {
    case EDITOR_ACTION_SAVE_SCENE:
        action_save_scene();
        break;
    case EDITOR_ACTION_REDO:
        action_redo();
        break;
    case EDITOR_ACTION_UNDO:
        action_undo();
        break;
    case EDITOR_ACTION_ADD_COMPONENT: // reinterpret as request event
    {
        if (selectedComp)
        {
            EditorRequestCreateComponentEvent event(selectedComp.suid());
            request_event(&event);
        }
        break;
    }
    case EDITOR_ACTION_CLONE_COMPONENT_SUBTREE:
        if (selectedComp)
            action_clone_component_subtree(selectedComp.suid());
        break;
    case EDITOR_ACTION_DELETE_COMPONENT_SUBTREE:
        if (selectedComp)
            action_delete_component_subtree(selectedComp.suid());
        break;
    case EDITOR_ACTION_CLOSE_DIALOG: // reinterpret as request event
    {
        EditorRequestCloseDialogEvent event{};
        request_event(&event);
        break;
    }
    default:
        break;
    }
}

void EditorContext::action_redo()
{
    (void)mObj->actionQueue.enqueue(EDITOR_ACTION_REDO);
}

void EditorContext::action_undo()
{
    (void)mObj->actionQueue.enqueue(EDITOR_ACTION_UNDO);
}

void EditorContext::action_new_scene(const FS::Path& sceneSchemaPath)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_NEW_SCENE);

    action->newScene = sceneSchemaPath;
}

void EditorContext::action_open_scene(const FS::Path& sceneSchemaPath)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_OPEN_SCENE);

    action->openScene = sceneSchemaPath;
}

void EditorContext::action_open_project(const FS::Path& projectSchemaPath)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_OPEN_PROJECT);

    action->openProject = projectSchemaPath;
}

void EditorContext::action_save_scene()
{
    (void)mObj->actionQueue.enqueue(EDITOR_ACTION_SAVE_SCENE);
}

void EditorContext::action_add_component(SUID parentSUID, ComponentType type)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT);

    action->addComponent.parentSUID = parentSUID;
    action->addComponent.compType = type;
}

void EditorContext::action_add_component_script(SUID compSUID, AssetID scriptAssetID)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_ADD_COMPONENT_SCRIPT);

    action->addComponentScript.compSUID = compSUID;
    action->addComponentScript.assetID = scriptAssetID;
}

void EditorContext::action_set_component_asset(SUID compSUID, AssetID assetID)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_SET_COMPONENT_ASSET);

    action->setComponentAsset.compSUID = compSUID;
    action->setComponentAsset.assetID = assetID;
}

void EditorContext::action_clone_component_subtree(SUID compSUID)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_CLONE_COMPONENT_SUBTREE);

    action->cloneComponentSubtree = compSUID;
}

void EditorContext::action_delete_component_subtree(SUID compSUID)
{
    EditorAction* action = mObj->actionQueue.enqueue(EDITOR_ACTION_DELETE_COMPONENT_SUBTREE);

    action->deleteComponentSubtree = compSUID;
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

void EditorContext::get_default_font(FontAtlas& fontAtlas, RImage& fontAtlasImage)
{
    fontAtlas = mObj->defaultFontAtlas;
    fontAtlasImage = mObj->defaultFontAtlasImage;
}

void EditorContext::get_mono_font(FontAtlas& fontAtlas, RImage& fontAtlasImage)
{
    fontAtlas = mObj->monoFontAtlas;
    fontAtlasImage = mObj->monoFontAtlasImage;
}

Scene EditorContext::get_scene()
{
    return mObj->scene;
}

Vector<RenderSystemScreenPass::Region> EditorContext::get_scene_screen_regions()
{
    Vector<Viewport> viewports;
    Vector<Rect> worldAABBs;
    mObj->scene.get_screen_regions(viewports, worldAABBs);
    LD_ASSERT(viewports.size() == worldAABBs.size());

    Vector<RenderSystemScreenPass::Region> regions(viewports.size());
    for (size_t i = 0; i < viewports.size(); i++)
    {
        regions[i].viewport = viewports[i];
        regions[i].worldAABB = worldAABBs[i];
    }

    return regions;
}

void EditorContext::add_observer(EditorEventFn fn, void* user)
{
    mObj->observers.add_observer(fn, user);
}

void EditorContext::input_key_value(KeyValue keyVal)
{
    if (!mObj->keyBinds.contains(keyVal))
        return;

    action(mObj->keyBinds[keyVal]);
}

void EditorContext::update(const Vec2& sceneExtent, float delta)
{
    LD_PROFILE_SCOPE;

    if (mObj->isPlaying)
    {
        mObj->scene.update(sceneExtent, delta);
    }
    else
    {
        mObj->scene.invalidate();
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

bool EditorContext::play_scene()
{
    LD_PROFILE_SCOPE;

    if (mObj->isPlaying)
        return true;

    // play a duplicated scene
    mObj->scene.backup();
    mObj->isPlaying = mObj->scene.startup();

    if (mObj->isPlaying)
    {
        mObj->prevSelectedComponentCUID = mObj->selectedComponentCUID;
    }

    return mObj->isPlaying;
}

void EditorContext::stop_scene()
{
    LD_PROFILE_SCOPE;

    if (!mObj->isPlaying)
        return;

    mObj->isPlaying = false;

    // restore original scene
    mObj->scene.cleanup();

    if (mObj->prevSelectedComponentCUID)
    {
        set_selected_component(mObj->prevSelectedComponentCUID);
        mObj->prevSelectedComponentCUID = 0;
    }
}

bool EditorContext::is_playing()
{
    return mObj->isPlaying;
}

void EditorContext::get_scene_roots(Vector<ComponentView>& roots)
{
    return mObj->scene.get_root_components(roots);
}

const char* EditorContext::get_component_name(CUID compCUID)
{
    ComponentView comp = mObj->scene.get_component(compCUID);
    if (!comp)
        return nullptr;

    return comp.get_name();
}

void EditorContext::request_event(const EditorRequestEvent* event)
{
    mObj->notify_observers(event);
}

void EditorContext::set_selected_component(CUID compCUID)
{
    if (mObj->selectedComponentCUID == compCUID)
        return;

    ComponentView comp = mObj->scene.get_component(compCUID);
    if (comp)
    {
        // update state and notify observers
        EditorNotifyComponentSelectionEvent event(compCUID);
        mObj->selectedComponentCUID = compCUID;
        mObj->notify_observers(&event);
    }
    else
    {
        mObj->selectedComponentCUID = 0;
    }
}

CUID EditorContext::get_selected_component()
{
    return mObj->selectedComponentCUID;
}

ComponentView EditorContext::get_selected_component_view()
{
    if (!mObj->selectedComponentCUID)
        return {};

    return mObj->scene.get_component(mObj->selectedComponentCUID);
}

ComponentView EditorContext::get_component(CUID compCUID)
{
    return mObj->scene.get_component(compCUID);
}

ComponentView EditorContext::get_component_by_suid(SUID compSUID)
{
    return mObj->scene.get_component_by_suid(compSUID);
}

ComponentView EditorContext::get_component_by_ruid(RUID ruid)
{
    return mObj->scene.get_ruid_component(ruid);
}

RUID EditorContext::get_selected_component_ruid()
{
    ComponentView comp = mObj->scene.get_component(mObj->selectedComponentCUID);

    return comp ? comp.ruid() : 0;
}

bool EditorContext::get_selected_component_transform(TransformEx& transform)
{
    ComponentView comp = mObj->scene.get_component(mObj->selectedComponentCUID);

    if (!comp)
        return false;

    return comp.get_transform(transform);
}

UILayoutInfo EditorContext::make_editor_window_layout(const Vec2& size)
{
    UILayoutInfo layoutI = mObj->settings.get_theme().make_vbox_layout();
    layoutI.sizeX = UISize::fixed(size.x);
    layoutI.sizeY = UISize::fixed(size.y);

    return layoutI;
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
