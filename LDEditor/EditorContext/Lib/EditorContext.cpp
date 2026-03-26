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
#include <Ludens/UI/UIFont.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorEventQueue.h>

#include <utility>

#include "EditorContextCommand.h"

namespace LD {

static Log sLog("EditorContext");

/// @brief Editor context implementation. Keeps track of the active project
///        and active scene states.
struct EditorContextObj
{
    RenderSystem renderSystem;                   /// render server handle
    AudioSystem audioSystem;                     /// audio server handle
    Image2D iconAtlas;                           /// editor icon atlas handle
    Project project;                             /// current project under edit
    Scene scene;                                 /// current scene under edit
    EditorSettings settings;                     /// editor global settings
    EditorEventQueue eventQueue;                 /// editor events waiting to be processed.
    EditStack editStack;                         /// undo/redo stack of EditCommands
    UIFontRegistry fontRegistry;                 /// all fonts used by editor are registered here
    UIFont fontDefault;                          /// editor default regular font
    UIFont fontMono;                             /// editor default monospace font
    DocumentRegistry docRegistry;                /// all documents to be displayed in the editor.
    FS::Path iconAtlasPath;                      /// path to editor icon atlas source file
    FS::Path sceneSchemaPath;                    /// path to current scene file
    FS::Path assetSchemaPath;                    /// path to project asset file
    FS::Path projectSchemaPath;                  /// path to project file
    Vector<FS::Path> scenePaths;                 /// path to scene schema files in project
    HashMap<KeyValue, EditorEventType> keyBinds; /// key shortcuts to generate events
    ObserverList<const EditorEvent*> observers;  /// all observers of EditorContext
    CUID selectedComponentCUID = 0;              /// component selection state
    CUID prevSelectedComponentCUID = 0;          /// previous component selection state
    bool isPlaying = false;                      /// whether Scene simulation is in progress

    void emit_event(EditorEventType type);
    void notify_observers(const EditorEvent* event);
    void load_project(const FS::Path& projectSchemaPath);
    void load_project_scene(const FS::Path& sceneSchemaPath);
    void new_project_scene(const FS::Path& sceneSchemaPath);
    void save_scene_schema();
    void save_project_schema();
};

static void editor_broadcast_event_handler(const EditorEvent* event, void* user);
static void editor_action_undo_event_handler(const EditorEvent* event, void* user);
static void editor_action_redo_event_handler(const EditorEvent* event, void* user);
static void editor_action_new_scene_event_handler(const EditorEvent* event, void* user);
static void editor_action_open_scene_event_handler(const EditorEvent* event, void* user);
static void editor_action_save_scene_event_handler(const EditorEvent* event, void* user);
static void editor_action_open_project_event_handler(const EditorEvent* event, void* user);
static void editor_action_add_component_event_handler(const EditorEvent* event, void* user);
static void editor_action_add_component_script_event_handler(const EditorEvent* event, void* user);
static void editor_action_set_component_asset_event_handler(const EditorEvent* event, void* user);
static void editor_action_clone_component_subtree_event_handler(const EditorEvent* event, void* user);
static void editor_action_delete_component_subtree_event_handler(const EditorEvent* event, void* user);

static struct
{
    EditorEventType type;
    EditorEventFn handler;
} sEditorEventHandlers[] = {
    {EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_DOCUMENT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_UNDO, &editor_action_undo_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_REDO, &editor_action_redo_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_NEW_SCENE, &editor_action_new_scene_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE, &editor_action_open_scene_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_SAVE_SCENE, &editor_action_save_scene_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT, &editor_action_open_project_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT, &editor_action_add_component_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT_SCRIPT, &editor_action_add_component_script_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET, &editor_action_set_component_asset_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE, &editor_action_clone_component_subtree_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE, &editor_action_delete_component_subtree_event_handler},
};

static void editor_broadcast_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event && user);

    // broadcast events to all observers of EditorContext
    auto* obj = (EditorContextObj*)user;
    obj->notify_observers(event);
}

static void editor_action_undo_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_UNDO);

    auto* obj = (EditorContextObj*)user;
    obj->editStack.undo();
}

static void editor_action_redo_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_REDO);

    auto* obj = (EditorContextObj*)user;
    obj->editStack.redo();
}

static void editor_action_new_scene_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_NEW_SCENE);
    EditorContextObj* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionNewSceneEvent*)event;

    // Creating new Scene would clear the EditStack
    obj->editStack.clear();

    obj->new_project_scene(e->newScene);
}

static void editor_action_open_scene_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE);
    EditorContextObj* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionOpenSceneEvent*)event;

    // Opening Scene would clear the EditStack
    obj->editStack.clear();

    obj->load_project_scene(e->openScene);
}

static void editor_action_save_scene_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_SAVE_SCENE);
    EditorContextObj* obj = (EditorContextObj*)user;
    (void)event;

    // Saving Scene writes the current schema to disk and
    // should not effect the EditStack.
    obj->save_scene_schema();
}

static void editor_action_open_project_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    auto* obj = (EditorContextObj*)user;

    // TODO: save scene and project dialog?
    // TODO: how much to unwind? AssetManager for sure, EditorContext invalidation?
    LD_UNREACHABLE;
}

static void editor_action_add_component_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionAddComponentEvent*)event;

    obj->editStack.execute(EditStack::new_command<AddComponentCommand>(
        obj->scene,
        e->parentSUID,
        e->compType));
}

static void editor_action_add_component_script_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT_SCRIPT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionAddComponentScriptEvent*)event;

    obj->editStack.execute(EditStack::new_command<AddComponentScriptCommand>(
        obj->scene,
        e->compSUID,
        e->assetID));
}

static void editor_action_set_component_asset_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionSetComponentAssetEvent*)event;

    obj->editStack.execute(EditStack::new_command<SetComponentAssetCommand>(
        obj->scene,
        e->compSUID,
        e->assetID));
}

static void editor_action_clone_component_subtree_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionCloneComponentSubtreeEvent*)event;

    obj->editStack.execute(EditStack::new_command<CloneComponentSubtreeCommand>(
        obj,
        e->compSUID));
}

static void editor_action_delete_component_subtree_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionDeleteComponentSubtreeEvent*)event;

    obj->editStack.execute(EditStack::new_command<DeleteComponentSubtreeCommand>(
        obj,
        e->compSUID));
}

void EditorContextObj::emit_event(EditorEventType type)
{
    ComponentView selectedComp = scene.get_component(selectedComponentCUID);

    switch (type)
    {
    case EDITOR_EVENT_TYPE_ACTION_REDO:
    case EDITOR_EVENT_TYPE_ACTION_UNDO:
    case EDITOR_EVENT_TYPE_ACTION_SAVE_SCENE:
    case EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG:
        (void)eventQueue.enqueue(type); // zero parameters needed
        break;
    case EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT:
    {
        if (selectedComp)
        {
            auto* event = (EditorRequestCreateComponentEvent*)eventQueue.enqueue(type);
            event->parent = selectedComp.suid();
        }
        break;
    }
    case EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE:
        if (selectedComp)
        {
            auto* event = (EditorActionCloneComponentSubtreeEvent*)eventQueue.enqueue(type);
            event->compSUID = selectedComp.suid();
        }
        break;
    case EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE:
        if (selectedComp)
        {
            auto* event = (EditorActionDeleteComponentSubtreeEvent*)eventQueue.enqueue(type);
            event->compSUID = selectedComp.suid();
        }
        break;
    default:
        break;
    }
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

    const FS::Path rootPath = project.get_root_path();
    sLog.info("loading project [{}], root directory {}", project.get_name(), rootPath.string());

    assetSchemaPath = project.get_asset_schema_absolute_path();
    if (!FS::exists(assetSchemaPath))
    {
        sLog.warn("failed to find project assets {}", assetSchemaPath.string());
        return;
    }

    AssetManagerInfo amI{};
    amI.rootPath = rootPath;
    amI.watchAssets = true;
    amI.assetSchemaPath = assetSchemaPath;
    AssetManager AM = AssetManager::create(amI);

    // Load all project assets at once using job system.
    // Once we have asynchronous-load-jobs maybe we can load assets
    // used by the loaded scene first?
    AM.begin_load_batch();
    AM.load_all_assets();

    Vector<std::string> errors;
    if (!AM.end_load_batch(errors))
    {
        sLog.error("AssetManager failed to load some assets, {} errors", errors.size());
        for (const std::string& err : errors)
            sLog.error("{}", err);
    }

    project.get_scene_schema_absolute_paths(scenePaths);

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
        sceneI.renderSystem = renderSystem;
        sceneI.audioSystem = audioSystem;
        sceneI.uiFont = fontDefault;
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
    obj->settings = EditorSettings::create();
    obj->isPlaying = false;
    obj->editStack = EditStack::create();
    obj->eventQueue = EditorEventQueue::create(obj);
    obj->fontRegistry = UIFontRegistry::create();
    obj->fontDefault = obj->fontRegistry.add_font(info.defaultFontAtlas, info.defaultFontAtlasImage);
    obj->fontMono = obj->fontRegistry.add_font(info.monoFontAtlas, info.monoFontAtlasImage);
    obj->docRegistry = DocumentRegistry::create();

    for (size_t i = 0; i < sizeof(sEditorEventHandlers) / sizeof(*sEditorEventHandlers); i++)
        register_editor_event_handler(sEditorEventHandlers[i].type, sEditorEventHandlers[i].handler);

    // register default global key binds
    obj->keyBinds[KeyValue(KEY_CODE_Z, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_UNDO;
    obj->keyBinds[KeyValue(KEY_CODE_R, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_REDO;
    obj->keyBinds[KeyValue(KEY_CODE_S, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_SAVE_SCENE;
    obj->keyBinds[KeyValue(KEY_CODE_A, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT;
    obj->keyBinds[KeyValue(KEY_CODE_D, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE;
    obj->keyBinds[KeyValue(KEY_CODE_DELETE)] = EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE;
    obj->keyBinds[KeyValue(KEY_CODE_ESCAPE)] = EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG;

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
    Scene::destroy();
    AssetManager::destroy();

    DocumentRegistry::destroy(obj->docRegistry);
    UIFontRegistry::destroy(obj->fontRegistry);
    EditorEventQueue::destroy(obj->eventQueue);
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

EditorEvent* EditorContext::enqueue_event(EditorEventType type)
{
    return mObj->eventQueue.enqueue(type);
}

void EditorContext::poll_events()
{
    mObj->eventQueue.poll_events();
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

UIFont EditorContext::get_font_default()
{
    LD_ASSERT(mObj->fontDefault);

    return mObj->fontDefault;
}

UIFont EditorContext::get_font_mono()
{
    LD_ASSERT(mObj->fontMono);

    return mObj->fontMono;
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

    mObj->emit_event(mObj->keyBinds[keyVal]);
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
    AssetManager::get().update();
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

void EditorContext::set_selected_component(CUID compCUID)
{
    if (mObj->selectedComponentCUID == compCUID)
        return;

    ComponentView comp = mObj->scene.get_component(compCUID);
    if (comp)
    {
        mObj->selectedComponentCUID = compCUID;

        // update state and notify observers
        auto* event = (EditorNotifyComponentSelectionEvent*)mObj->eventQueue.enqueue(EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION);
        event->cuid = compCUID;
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

} // namespace LD
