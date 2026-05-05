#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Observer.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectContext.h>
#include <Ludens/Project/ProjectLoadAsync.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Timer.h>
#include <Ludens/UI/UIFont.h>
#include <LudensBuilder/AssetBuilder/AssetBuilder.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>
#include <LudensBuilder/ProjectBuilder/ProjectBuilder.h>
#include <LudensBuilder/ProjectBuilder/ProjectScan.h>
#include <LudensEditor/EditorContext/EditCommand.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorEventQueue.h>

// TODO: development builds only
#include <LudensUtil/TestUtil/TestUtil.h>

#include <utility>

#include "EditorContextObj.h"

namespace LD {

static Log sLog("EditorContext");

static void editor_broadcast_event_handler(const EditorEvent* event, void* user);
static void editor_notify_project_settings_dirty_event_handler(const EditorEvent* event, void* user);
static void editor_notify_file_drop_event_handler(const EditorEvent* event, void* user);
static void editor_action_save_event_handler(const EditorEvent* event, void* user);
static void editor_action_undo_event_handler(const EditorEvent* event, void* user);
static void editor_action_redo_event_handler(const EditorEvent* event, void* user);
static void editor_action_open_scene_event_handler(const EditorEvent* event, void* user);
static void editor_action_open_project_event_handler(const EditorEvent* event, void* user);
static void editor_action_create_project_event_handler(const EditorEvent* event, void* user);
static void editor_action_create_scene_event_handler(const EditorEvent* event, void* user);
static void editor_action_import_assets_event_handler(const EditorEvent* event, void* user);
static void editor_action_import_assets_async_event_handler(const EditorEvent* event, void* user);
static void editor_action_rename_asset_event_handler(const EditorEvent* event, void* user);
static void editor_action_rename_scene_event_handler(const EditorEvent* event, void* user);
static void editor_action_rename_component_event_handler(const EditorEvent* event, void* user);
static void editor_action_add_component_event_handler(const EditorEvent* event, void* user);
static void editor_action_set_component_script_event_handler(const EditorEvent* event, void* user);
static void editor_action_set_component_asset_event_handler(const EditorEvent* event, void* user);
static void editor_action_set_component_props_event_handler(const EditorEvent* event, void* user);
static void editor_action_clone_component_subtree_event_handler(const EditorEvent* event, void* user);
static void editor_action_delete_component_subtree_event_handler(const EditorEvent* event, void* user);

static struct
{
    EditorEventType type;
    EditorEventFn handler;
} sEditorEventHandlers[] = {
    {EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY, &editor_notify_project_settings_dirty_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP, &editor_notify_file_drop_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_SHOW_MODAL, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_HIDE_MODAL, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_WORKSPACE_LAYOUT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_COMPONENT_SCRIPT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_CREATE_SCENE, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_CREATE_PROJECT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_REQUEST_DOCUMENT, &editor_broadcast_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_SAVE, &editor_action_save_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_UNDO, &editor_action_undo_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_REDO, &editor_action_redo_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE, &editor_action_open_scene_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT, &editor_action_open_project_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_CREATE_SCENE, &editor_action_create_scene_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT, &editor_action_create_project_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS, &editor_action_import_assets_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS_ASYNC, &editor_action_import_assets_async_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_RENAME_ASSET, &editor_action_rename_asset_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_RENAME_SCENE, &editor_action_rename_scene_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_RENAME_COMPONENT, &editor_action_rename_component_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT, &editor_action_add_component_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_SCRIPT, &editor_action_set_component_script_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET, &editor_action_set_component_asset_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_PROPS, &editor_action_set_component_props_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE, &editor_action_clone_component_subtree_event_handler},
    {EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE, &editor_action_delete_component_subtree_event_handler},
};

static_assert(sizeof(sEditorEventHandlers) / sizeof(*sEditorEventHandlers) == EDITOR_EVENT_TYPE_ENUM_COUNT);

static void editor_broadcast_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event && user);

    // broadcast events to all observers of EditorContext
    auto* obj = (EditorContextObj*)user;
    obj->notify_observers(event);
}

static void editor_notify_project_settings_dirty_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY);
    EditorContextObj* obj = (EditorContextObj*)user;
    auto* e = (const EditorNotifyProjectSettingsDirtyEvent*)event;

    if (e->dirtyScreenLayers)
        obj->projectCtx.configure_project_screen_layers();
}

static void editor_notify_file_drop_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP);
    EditorContextObj* obj = (EditorContextObj*)user;
    auto* e = (const EditorNotifyFileDropEvent*)event;

    if (e->files.empty())
        return;

    // Interpret dropped files.
    // Most common use case is to begin importing assets.

    const FS::Path& file = e->files.front();
    auto* requestE = (EditorRequestImportAssetsEvent*)obj->eventQueue.enqueue(EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS);
    requestE->srcPath = file;

    for (size_t i = 1; i < e->files.size(); i++)
    {
        sLog.warn("ignored file drop: {}", e->files[i].string());
    }
}

static void editor_action_save_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_SAVE);
    EditorContextObj* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionSaveEvent*)event;

    // NOTE: Currently this is synchronous, probably want to make this async later.

    if (e->saveAssetSchema)
        obj->save_asset_schema();
    if (e->saveSceneSchema)
        obj->save_scene_schema();
    if (e->saveProjectSchema)
        obj->save_project_schema();

    obj->lastSavedEditIndex = obj->editStack.index();
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

static void editor_action_open_scene_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE);
    EditorContextObj* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionOpenSceneEvent*)event;

    // Opening Scene would clear the EditStack
    obj->editStack.clear();
    obj->lastSavedEditIndex = 0;

    obj->load_project_scene(e->sceneID);
}

static void editor_action_open_project_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionOpenProjectEvent*)event;

    // TODO: save scene and project dialog?

    obj->begin_project_load_async(e->projectSchema);
}

static void editor_action_create_project_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionCreateProjectEvent*)event;
    EditorContext ctx(obj);

    auto* notify = (EditorNotifyProjectCreationEvent*)ctx.enqueue_event(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION);
    if (create_empty_project(e->projectName, e->projectSchema, notify->error))
        notify->projectSchema = e->projectSchema;
}

static void editor_action_create_scene_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_CREATE_SCENE);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionCreateSceneEvent*)event;

    Project project = obj->projectCtx.project();
    SUIDRegistry suidReg = obj->projectCtx.suid_registry();

    std::string err;
    FS::Path sceneSchemaAbsPath;
    SUID sceneID = project.register_scene(suidReg, e->scenePath, err);
    if (!sceneID || !project.get_scene_schema_abs_path(sceneID, sceneSchemaAbsPath))
        return;

    if (!create_empty_scene(sceneSchemaAbsPath, err))
        project.unregister_scene(suidReg, sceneID);

    // try load newly created scene immediately
    obj->load_project_scene(sceneID);
}

static void editor_action_import_assets_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionImportAssetsEvent*)event;

    // TODO: what if async batch is already in progress

    obj->assetImporter.set_resolve_params(obj->projectCtx.suid_registry(), obj->projectCtx.root_dir_abs_path());

    // blocking import on main thread.
    for (AssetImportInfo* info : e->batch)
    {
        (void)obj->assetImporter.import_asset_synchronous(info);
    }
}

static void editor_action_import_assets_async_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS_ASYNC);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionImportAssetsAsyncEvent*)event;

    // TODO: what if batch is already in progress

    obj->assetImporter.set_resolve_params(obj->projectCtx.suid_registry(), obj->projectCtx.root_dir_abs_path());
    obj->assetImporter.import_batch_begin();

    for (AssetImportInfo* info : e->batch)
        obj->assetImporter.import_batch_asset(info);

    obj->assetImporter.import_batch_end();
}

void editor_action_rename_asset_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_RENAME_ASSET);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionRenameAssetEvent*)event;

    auto* cmd = (RenameAssetCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_RENAME_ASSET);
    cmd->configure(e->assetID, e->newPath);
    obj->editStack.execute(cmd);
}

static void editor_action_rename_scene_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_RENAME_SCENE);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionRenameSceneEvent*)event;

    auto* cmd = (RenameSceneCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_RENAME_SCENE);
    cmd->configure(e->sceneID, e->newPath);
    obj->editStack.execute(cmd);
}

static void editor_action_rename_component_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_RENAME_COMPONENT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionRenameComponentEvent*)event;

    auto* cmd = (RenameComponentCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_RENAME_COMPONENT);
    cmd->configure(e->compSUID, e->newName);
    obj->editStack.execute(cmd);
}

static void editor_action_add_component_event_handler(const EditorEvent* event, void* user)
{
    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionAddComponentEvent*)event;

    auto* cmd = (AddComponentCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_ADD_COMPONENT);
    cmd->configure(e->parentSUID, e->compType);
    obj->editStack.execute(cmd);
}

static void editor_action_set_component_script_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_SCRIPT);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionSetComponentScriptEvent*)event;

    auto* cmd = (SetComponentScriptCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_SET_COMPONENT_SCRIPT);
    cmd->configure(e->compSUID, e->assetID);
    obj->editStack.execute(cmd);
}

static void editor_action_set_component_asset_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionSetComponentAssetEvent*)event;

    auto* cmd = (SetComponentAssetCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_SET_COMPONENT_ASSET);
    cmd->configure(e->compSUID, e->assetID, e->assetSlotIndex);
    obj->editStack.execute(cmd);
}

static void editor_action_set_component_props_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_PROPS);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionSetComponentPropsEvent*)event;

    auto* cmd = (SetComponentPropsCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_SET_COMPONENT_PROPS);
    cmd->compSUID = e->compSUID;
    cmd->delta = e->delta;
    obj->editStack.execute(cmd);
}

static void editor_action_clone_component_subtree_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionCloneComponentSubtreeEvent*)event;

    auto* cmd = (CloneComponentSubtreeCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_CLONE_COMPONENT_SUBTREE);
    cmd->configure(e->compSUID);
    obj->editStack.execute(cmd);
}

static void editor_action_delete_component_subtree_event_handler(const EditorEvent* event, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(event->type == EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE);
    auto* obj = (EditorContextObj*)user;
    auto* e = (const EditorActionDeleteComponentSubtreeEvent*)event;

    auto* cmd = (DeleteComponentSubtreeCommand*)obj->editStack.allocate(EDIT_COMMAND_TYPE_DELETE_COMPONENT_SUBTREE);
    cmd->configure(e->compSUID);
    obj->editStack.execute(cmd);
}

void EditorContextObj::emit_event(EditorEventType type)
{
    ComponentView selectedComp = {};

    if (scene)
        selectedComp = scene.get_component(selectedComponentCUID);

    switch (type)
    {
    case EDITOR_EVENT_TYPE_ACTION_SAVE: // interpret as save everything
    {
        auto* actionE = (EditorActionSaveEvent*)eventQueue.enqueue(type);
        actionE->saveAssetSchema = true;
        actionE->saveSceneSchema = true;
        actionE->saveProjectSchema = true;
        break;
    }
    case EDITOR_EVENT_TYPE_ACTION_REDO:
    case EDITOR_EVENT_TYPE_ACTION_UNDO:
    case EDITOR_EVENT_TYPE_REQUEST_HIDE_MODAL:
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE:
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT:
    case EDITOR_EVENT_TYPE_REQUEST_CREATE_SCENE:
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

void EditorContextObj::load_project_scene(SUID sceneID)
{
    LD_PROFILE_SCOPE;

    if (activeSceneID == sceneID)
        return;

    Project project = projectCtx.project();
    LD_ASSERT(project);

    FS::Path nextSceneSchemaPath;
    if (!project || !project.get_scene_schema_abs_path(sceneID, nextSceneSchemaPath) || !FS::exists(nextSceneSchemaPath))
        return;

    projectCtx.configure_project_screen_layers();

    scene = Scene::get();
    LD_ASSERT(scene);
    scene.unload();

    // TODO: transactional
    scene.load([&](SceneObj* sceneObj) -> bool {
        // load the scene
        std::string err;
        return SceneSchema::load_scene_from_file(Scene(sceneObj), projectCtx.suid_registry(), nextSceneSchemaPath, err);
    });

    editStack.clear();

    activeSceneID = sceneID;
    sceneSchemaAbsPath = nextSceneSchemaPath;
}

void EditorContextObj::save_scene_schema()
{
    if (!scene || sceneSchemaAbsPath.empty())
        return;

    Timer timer;
    timer.start();

    std::string err;
    if (!SceneSchema::save_scene(scene, sceneSchemaAbsPath, err))
    {
        sLog.error("failed to save scene to [{}]: {}", sceneSchemaAbsPath.string(), err);
    }
    else
    {
        size_t us = timer.stop();
        sLog.info("saved scene ({} ms)", us / 1000.0f);
    }
}

void EditorContextObj::save_asset_schema()
{
    Timer timer;
    timer.start();

    std::string err;
    std::string assetSchemaPathStr = projectCtx.asset_schema_abs_path().string();

    if (!projectCtx.save_asset_registry(err))
    {
        sLog.error("failed to save asset registry to [{}]: {}", assetSchemaPathStr, err);
    }
    else
    {
        size_t us = timer.stop();
        sLog.info("saved asset registry ({} ms)", us / 1000.0f);
    }
}

void EditorContextObj::save_project_schema()
{
    Timer timer;
    timer.start();

    std::string err;
    std::string projectSchemaPathStr = projectCtx.asset_schema_abs_path().string();

    if (!projectCtx.save_project(err))
    {
        sLog.error("failed to save project to [{}]: {}", projectSchemaPathStr, err);
    }
    else
    {
        size_t us = timer.stop();
        sLog.info("saved project ({} ms)", us / 1000.0f);
    }
}

void EditorContextObj::begin_project_load_async(const FS::Path& projectSchemaPath)
{
    LD_PROFILE_SCOPE;

    std::string err;
    const FS::Path rootPath = projectSchemaPath.parent_path();

    if (projectLoadAsync)
    {
        if (projectLoadAsync.update() != PROJECT_LOAD_STATE_IDLE)
            return;

        ProjectLoadAsync::destroy(projectLoadAsync);
    }

    projectLoadAsync = ProjectLoadAsync::create(&projectCtx);
    if (!projectLoadAsync.begin(rootPath, err))
    {
        sLog.error("failed to load project {}\n{}", projectSchemaPath.string(), err);
        LD_DEBUG_BREAK;
        return;
    }
}

void EditorContextObj::update_project_load_async()
{
    if (!projectLoadAsync || projectLoadAsync.update() != PROJECT_LOAD_STATE_COMPLETE)
        return;

    ProjectLoadAsync::destroy(projectLoadAsync);
    projectLoadAsync = {};
    selectedComponentCUID = 0;

    // TODO: handle errors
    (void*)eventQueue.enqueue(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD);

    SUID defaultSceneID = projectCtx.project().get_default_scene_id();
    LD_ASSERT(defaultSceneID);

    load_project_scene(defaultSceneID);
    // (void*)eventQueue.enqueue(EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD);
}

void EditorContextObj::update_asset_import_async()
{
    AssetImportResult result;
    if (!assetImporter || !assetImporter.import_asset_async(result))
        return;

    if (!result.status)
        sLog.info("Asset import failed: {}", result.status.str);
}

void EditorContextObj::add_project_entry(const ProjectScanResult& scanResult)
{
    if (!scanResult.isProjectSchemaValid)
        return;

    EditorProjectEntry entry{};
    entry.projectName = scanResult.projectName;
    entry.schemaPath = scanResult.projectSchema;
    projectEntries[entry.schemaPath] = entry;
}

bool EditorContextObj::prepare_document(const char* uriPath)
{
    if (docRegistry.get_document(uriPath))
        return true;

    // SPACE: Load Markdown documents embedded in LDEditor binary.
    //        For development builds we just load from disk.

    FS::Path relPath = document_md_path_from_uri_path(uriPath);
    if (relPath.empty())
        return false;

    FS::Path path;
    if (!TestUtil::get_root_directory_path(&path))
        return false;

    path = FS::absolute(path / "Docs" / relPath);

    std::string err;
    Vector<byte> v;
    if (!FS::read_file_to_vector(path, v, err))
        return false;

    DocumentInfo docI{};
    docI.uriPath = uriPath;
    docI.md = view(v);
    docI.copyData = true;
    return docRegistry.add_document(docI, err);
}

EditorContext EditorContext::create(const EditorContextInfo& info)
{
    LD_PROFILE_SCOPE;

    EditorContextObj* obj = heap_new<EditorContextObj>(MEMORY_USAGE_MISC);
    obj->renderSystem = info.renderSystem;
    obj->audioSystem = info.audioSystem;
    obj->iconAtlasPath = info.iconAtlasPath;
    obj->assetBuilder = AssetBuilder::create();
    obj->assetImporter = AssetImporter::create();
    obj->settings = EditorSettings::create();
    obj->isPlaying = false;
    obj->editStack = EditStack::create(obj);
    obj->lastSavedEditIndex = obj->editStack.index();
    obj->eventQueue = EditorEventQueue::create(obj);
    obj->fontRegistry = UIFontRegistry::create();
    obj->fontDefault = obj->fontRegistry.add_font(info.defaultFontAtlas, info.defaultFontAtlasImage);
    obj->fontMono = obj->fontRegistry.add_font(info.monoFontAtlas, info.monoFontAtlasImage);
    obj->docRegistry = DocumentRegistry::create();

    for (size_t i = 0; i < info.projectScanResultCount; i++)
        obj->add_project_entry(info.projectScanResults[i]);

    for (size_t i = 0; i < sizeof(sEditorEventHandlers) / sizeof(*sEditorEventHandlers); i++)
        register_editor_event_handler(sEditorEventHandlers[i].type, sEditorEventHandlers[i].handler);

    // register default global key binds
    obj->keyBinds[KeyValue(KEY_CODE_S, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_SAVE;
    obj->keyBinds[KeyValue(KEY_CODE_Z, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_UNDO;
    obj->keyBinds[KeyValue(KEY_CODE_R, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_REDO;
    obj->keyBinds[KeyValue(KEY_CODE_N, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_REQUEST_CREATE_SCENE;
    obj->keyBinds[KeyValue(KEY_CODE_O, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE;
    obj->keyBinds[KeyValue(KEY_CODE_O, KEY_MOD_CONTROL_BIT | KEY_MOD_SHIFT_BIT)] = EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT;
    obj->keyBinds[KeyValue(KEY_CODE_A, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT;
    obj->keyBinds[KeyValue(KEY_CODE_D, KEY_MOD_CONTROL_BIT)] = EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE;
    obj->keyBinds[KeyValue(KEY_CODE_DELETE)] = EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE;
    obj->keyBinds[KeyValue(KEY_CODE_ESCAPE)] = EDITOR_EVENT_TYPE_REQUEST_HIDE_MODAL;

    AssetManagerInfo amI{};
    amI.watchAssets = true;
    AssetManager::create(amI);

    SceneInfo sceneI{};
    sceneI.renderSystem = obj->renderSystem;
    sceneI.audioSystem = obj->audioSystem;
    sceneI.uiFont = obj->fontDefault;
    sceneI.uiTheme = obj->settings.get_theme().get_ui_theme();
    sceneI.suidRegistry = obj->projectCtx.suid_registry();
    (void)Scene::create(sceneI);

    obj->scene = {};

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

    Scene::destroy();
    AssetManager::destroy();

    DocumentRegistry::destroy(obj->docRegistry);
    UIFontRegistry::destroy(obj->fontRegistry);
    EditorEventQueue::destroy(obj->eventQueue);
    EditStack::destroy(obj->editStack);
    EditorSettings::destroy(obj->settings);
    AssetImporter::destroy(obj->assetImporter);
    AssetBuilder::destroy(obj->assetBuilder);

    heap_delete<EditorContextObj>(obj);
}

bool EditorContext::render_system_mat4_callback(RUID ruid, Mat4& mat4, void* user)
{
    EditorContextObj& self = *(EditorContextObj*)user;

    if (!self.scene)
        return false;

    return self.scene.get_ruid_world_mat4(ruid, mat4);
}

void EditorContext::drop_file_callback(size_t fileCount, const FS::Path* files, void* user)
{
    // We defer heavy I/O since this is within an OS callback.
    // The EditorEventQueue is perfect for temporal decoupling.
    EditorContextObj* obj = (EditorContextObj*)user;

    auto* notifyE = (EditorNotifyFileDropEvent*)obj->eventQueue.enqueue(EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP);
    notifyE->files.resize(fileCount);

    std::copy(files, files + fileCount, notifyE->files.begin());
}

EditorEvent* EditorContext::enqueue_event(EditorEventType type)
{
    return mObj->eventQueue.enqueue(type);
}

void EditorContext::poll_events()
{
    mObj->eventQueue.poll_events();
}

AssetImportInfo* EditorContextAssetInterface::alloc_asset_import_info(AssetType type)
{
    return mObj->assetImporter.allocate_import_info(type);
}

void EditorContextAssetInterface::free_asset_import_info(AssetImportInfo* info)
{
    mObj->assetImporter.free_import_info(info);
}

AssetCreateInfo* EditorContextAssetInterface::alloc_asset_create_info(AssetCreateType type)
{
    return mObj->assetBuilder.allocate_create_info(type);
}

void EditorContextAssetInterface::free_asset_create_info(AssetCreateInfo* info)
{
    mObj->assetBuilder.free_create_info(info);
}

Asset EditorContextAssetInterface::create_asset(AssetCreateInfo* info, const std::string importDstPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    AssetBuilder builder = mObj->assetBuilder;
    AssetImporter importer = mObj->assetImporter;

    if (!builder.create_asset(info, err))
        return {};

    AssetImportInfo* importInfo = importer.allocate_import_info(info->assetType);
    importInfo->dstPath = importDstPath;

    Asset dstAsset = {};

    if (builder.prepare_import(info, importInfo, err))
    {
        importer.set_resolve_params(mObj->projectCtx.suid_registry(), mObj->projectCtx.project().get_storage_dir_abs_path());

        AssetImportResult result = importer.import_asset_synchronous(importInfo);
        if (result.status.type == ASSET_IMPORT_SUCCESS && result.dstAsset)
            dstAsset = result.dstAsset;
        else
            err = result.status.str;
    }

    builder.free_create_info(info);

    return dstAsset;
}

bool EditorContext::is_project_dirty()
{
    return mObj->editStack.index() != mObj->lastSavedEditIndex;
}

Vector<EditorProjectEntry> EditorContext::get_project_entries()
{
    Vector<EditorProjectEntry> entries;

    for (const auto& it : mObj->projectEntries)
        entries.push_back(it.second);

    return entries;
}

Vector<SUIDEntry> EditorContext::get_project_scene_entries()
{
    Project project = mObj->projectCtx.project();
    if (!project)
        return {};

    Vector<SUIDEntry> entries;
    project.get_scenes(entries);
    return entries;
}

FS::Path EditorContext::get_project_directory()
{
    return mObj->projectCtx.project_schema_abs_path().parent_path();
}

FS::Path EditorContext::get_scene_schema_path()
{
    return mObj->sceneSchemaAbsPath;
}

EditorSettings EditorContext::settings()
{
    return mObj->settings;
}

Project EditorContext::get_project()
{
    return mObj->projectCtx.project();
}

ProjectSettings EditorContext::get_project_settings()
{
    return mObj->projectCtx.project().settings();
}

SUIDRegistry EditorContext::get_suid_registry()
{
    return mObj->projectCtx.suid_registry();
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
    if (!mObj->scene)
        return {};

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

Camera EditorContext::get_scene_camera()
{
    Scene scene = get_scene();

    return scene ? scene.get_camera() : Camera();
}

Vec4 EditorContext::get_scene_clear_color()
{
    ProjectSettings settings = get_project_settings();

    if (settings)
        return settings.rendering_settings().get_clear_color();

    return ProjectRenderingSettings::get_default_clear_color();
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

    mObj->update_project_load_async();
    mObj->update_asset_import_async();

    if (!mObj->scene)
        return;

    if (mObj->isPlaying)
    {
        SceneUpdateTick tick{};
        tick.extent = sceneExtent;
        tick.delta = delta;
        mObj->scene.update(tick);
    }
    else
    {
        mObj->scene.invalidate(sceneExtent);
    }

    // NOTE: this polls for any asset file changes.
    AssetManager::get().update();
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
    roots.clear();

    if (mObj->scene)
        mObj->scene.get_root_components(roots);
}

const char* EditorContext::get_component_name(CUID compCUID)
{
    if (!mObj->scene)
        return nullptr;

    ComponentView comp = mObj->scene.get_component(compCUID);
    if (!comp)
        return nullptr;

    return comp.get_name();
}

void EditorContext::set_selected_component(CUID compCUID)
{
    if (!mObj->scene || mObj->selectedComponentCUID == compCUID)
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
    if (!mObj->scene || !mObj->selectedComponentCUID)
        return {};

    return mObj->scene.get_component(mObj->selectedComponentCUID);
}

ComponentView EditorContext::get_component(CUID compCUID)
{
    if (!mObj->scene)
        return {};

    return mObj->scene.get_component(compCUID);
}

ComponentView EditorContext::get_component_by_suid(SUID compSUID)
{
    if (!mObj->scene)
        return {};

    return mObj->scene.get_component_by_suid(compSUID);
}

ComponentView EditorContext::get_component_by_ruid(RUID ruid)
{
    if (!mObj->scene)
        return {};

    return mObj->scene.get_ruid_component(ruid);
}

RUID EditorContext::get_selected_component_ruid()
{
    if (!mObj->scene)
        return RUID(0);

    ComponentView comp = mObj->scene.get_component(mObj->selectedComponentCUID);

    return comp ? comp.ruid() : 0;
}

bool EditorContext::get_selected_component_transform(TransformEx& transform)
{
    if (!mObj->scene)
        return false;

    ComponentView comp = mObj->scene.get_component(mObj->selectedComponentCUID);

    if (!comp)
        return false;

    return comp.get_transform(transform);
}

Document EditorContext::get_document(const char* uriPath)
{
    mObj->prepare_document(uriPath);

    return mObj->docRegistry.get_document(uriPath);
}

} // namespace LD
