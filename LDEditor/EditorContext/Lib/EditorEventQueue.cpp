#include <Ludens/DSA/Queue.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensEditor/EditorContext/EditorEventQueue.h>

namespace LD {

union EditorEventU
{
    EditorNotifyProjectCreationEvent notifyProjectCreation;
    EditorNotifyProjectLoadEvent notifyProjectLoad;
    EditorNotifyProjectSettingsDirtyEvent notifyProjectSettingsDirty;
    EditorNotifySceneLoadEvent notifySceneLoad;
    EditorNotifyComponentSelectionEvent notifyComponentSelect;
    EditorNotifyFileDropEvent notifyFileDrop;
    EditorRequestCloseDialogEvent requestCloseDialog;
    EditorRequestProjectSettingsEvent requestProjectSettings;
    EditorRequestComponentAssetEvent requestComponentAsset;
    EditorRequestImportAssetsEvent requestImportAssets;
    EditorRequestNewProjectEvent requestNewProject;
    EditorRequestOpenProjectEvent requestOpenProject;
    EditorRequestNewSceneEvent requestNewScene;
    EditorRequestOpenSceneEvent requestOpenScene;
    EditorRequestCreateComponentEvent requestCreateComponent;
    EditorRequestDocumentEvent requestDocument;
    EditorActionSaveEvent actionSave;
    EditorActionUndoEvent actionUndo;
    EditorActionRedoEvent actionRedo;
    EditorActionNewSceneEvent actionNewScene;
    EditorActionOpenSceneEvent actionOpenScene;
    EditorActionOpenProjectEvent actionOpenProject;
    EditorActionCreateProjectEvent actionCreateProject;
    EditorActionImportAssetsEvent actionImportAssets;
    EditorActionAddComponentEvent actionAddComponent;
    EditorActionAddComponentScriptEvent actionAddComponentScript;
    EditorActionSetComponentAssetEvent actionSetComponentAsset;
    EditorActionCloneComponentSubtreeEvent actionCloneComponentSubtree;
    EditorActionDeleteComponentSubtreeEvent actionDeleteComponentSubtree;
};

struct EditorEventQueueObj
{
    Queue<EditorEvent*> queue;
    PoolAllocator eventPA;
    void* user;

    EditorEvent* alloc_event(EditorEventType type);
    void free_event(EditorEvent* event);
};

struct EditorEventMeta
{
    EditorEventFn handler;
};

static EditorEventMeta sEditorEventMeta[EDITOR_EVENT_TYPE_ENUM_COUNT];

static_assert(sizeof(sEditorEventMeta) / sizeof(*sEditorEventMeta) == (int)EDITOR_EVENT_TYPE_ENUM_COUNT);

// TODO: The switch statement for alloc/free can not be flattened by compiler,
//       maybe manually flatten into tables of function pointers per-type?
//       the only polymorphism required for EditorEvents is ctor and dtor.
EditorEvent* EditorEventQueueObj::alloc_event(EditorEventType type)
{
    EditorEventU* event = (EditorEventU*)eventPA.allocate();

    switch (type)
    {
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION:
        new (event) EditorNotifyProjectCreationEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD:
        new (event) EditorNotifyProjectLoadEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY:
        new (event) EditorNotifyProjectSettingsDirtyEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD:
        new (event) EditorNotifySceneLoadEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION:
        new (event) EditorNotifyComponentSelectionEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP:
        new (event) EditorNotifyFileDropEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG:
        new (event) EditorRequestCloseDialogEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS:
        new (event) EditorRequestProjectSettingsEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET:
        new (event) EditorRequestComponentAssetEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS:
        new (event) EditorRequestImportAssetsEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT:
        new (event) EditorRequestNewProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT:
        new (event) EditorRequestOpenProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE:
        new (event) EditorRequestNewSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE:
        new (event) EditorRequestOpenSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT:
        new (event) EditorRequestCreateComponentEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_DOCUMENT:
        new (event) EditorRequestDocumentEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_SAVE:
        new (event) EditorActionSaveEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_UNDO:
        new (event) EditorActionUndoEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_REDO:
        new (event) EditorActionRedoEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_NEW_SCENE:
        new (event) EditorActionNewSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE:
        new (event) EditorActionOpenSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT:
        new (event) EditorActionOpenProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT:
        new (event) EditorActionCreateProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS:
        new (event) EditorActionImportAssetsEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT:
        new (event) EditorActionAddComponentEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT_SCRIPT:
        new (event) EditorActionAddComponentScriptEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET:
        new (event) EditorActionSetComponentAssetEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE:
        new (event) EditorActionCloneComponentSubtreeEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE:
        new (event) EditorActionDeleteComponentSubtreeEvent();
        break;
    default:
        LD_DEBUG_BREAK;
        break;
    }

    return (EditorEvent*)event;
}

void EditorEventQueueObj::free_event(EditorEvent* event)
{
    switch (event->type)
    {
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION:
        ((EditorNotifyProjectCreationEvent*)(event))->~EditorNotifyProjectCreationEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD:
        ((EditorNotifyProjectLoadEvent*)(event))->~EditorNotifyProjectLoadEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY:
        ((EditorNotifyProjectSettingsDirtyEvent*)(event))->~EditorNotifyProjectSettingsDirtyEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD:
        ((EditorNotifySceneLoadEvent*)(event))->~EditorNotifySceneLoadEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION:
        ((EditorNotifyComponentSelectionEvent*)(event))->~EditorNotifyComponentSelectionEvent();
        break;
    case EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP:
        ((EditorNotifyFileDropEvent*)event)->~EditorNotifyFileDropEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG:
        ((EditorRequestCloseDialogEvent*)(event))->~EditorRequestCloseDialogEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS:
        ((EditorRequestProjectSettingsEvent*)(event))->~EditorRequestProjectSettingsEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET:
        ((EditorRequestComponentAssetEvent*)(event))->~EditorRequestComponentAssetEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS:
        ((EditorRequestImportAssetsEvent*)(event))->~EditorRequestImportAssetsEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT:
        ((EditorRequestNewProjectEvent*)(event))->~EditorRequestNewProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT:
        ((EditorRequestOpenProjectEvent*)(event))->~EditorRequestOpenProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE:
        ((EditorRequestNewSceneEvent*)(event))->~EditorRequestNewSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE:
        ((EditorRequestOpenSceneEvent*)(event))->~EditorRequestOpenSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT:
        ((EditorRequestCreateComponentEvent*)(event))->~EditorRequestCreateComponentEvent();
        break;
    case EDITOR_EVENT_TYPE_REQUEST_DOCUMENT:
        ((EditorRequestDocumentEvent*)(event))->~EditorRequestDocumentEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_SAVE:
        ((EditorActionSaveEvent*)(event))->~EditorActionSaveEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_UNDO:
        ((EditorActionUndoEvent*)(event))->~EditorActionUndoEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_REDO:
        ((EditorActionRedoEvent*)(event))->~EditorActionRedoEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_NEW_SCENE:
        ((EditorActionNewSceneEvent*)(event))->~EditorActionNewSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE:
        ((EditorActionOpenSceneEvent*)(event))->~EditorActionOpenSceneEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT:
        ((EditorActionOpenProjectEvent*)(event))->~EditorActionOpenProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT:
        ((EditorActionCreateProjectEvent*)(event))->~EditorActionCreateProjectEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS:
        ((EditorActionImportAssetsEvent*)event)->~EditorActionImportAssetsEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT:
        ((EditorActionAddComponentEvent*)(event))->~EditorActionAddComponentEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT_SCRIPT:
        ((EditorActionAddComponentScriptEvent*)(event))->~EditorActionAddComponentScriptEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET:
        ((EditorActionSetComponentAssetEvent*)(event))->~EditorActionSetComponentAssetEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE:
        ((EditorActionCloneComponentSubtreeEvent*)(event))->~EditorActionCloneComponentSubtreeEvent();
        break;
    case EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE:
        ((EditorActionDeleteComponentSubtreeEvent*)(event))->~EditorActionDeleteComponentSubtreeEvent();
        break;
    default:
        LD_DEBUG_BREAK;
        break;
    }

    eventPA.free(event);
}

EditorEventQueue EditorEventQueue::create(void* user)
{
    auto* obj = heap_new<EditorEventQueueObj>(MEMORY_USAGE_MISC);
    obj->user = user;

    PoolAllocatorInfo paI{};
    paI.isMultiPage = true;
    paI.blockSize = sizeof(EditorEventU);
    paI.pageSize = 1024;
    paI.usage = MEMORY_USAGE_MISC;
    obj->eventPA = PoolAllocator::create(paI);

    return EditorEventQueue(obj);
}

void EditorEventQueue::destroy(EditorEventQueue queue)
{
    auto* obj = queue.unwrap();

    PoolAllocator::destroy(obj->eventPA);

    heap_delete<EditorEventQueueObj>(obj);
}

EditorEvent* EditorEventQueue::enqueue(EditorEventType type)
{
    EditorEvent* event = mObj->alloc_event(type);

    mObj->queue.push(event);

    return event;
}

void EditorEventQueue::poll_events()
{
    LD_PROFILE_SCOPE;

    while (!mObj->queue.empty())
    {
        EditorEvent* event = mObj->queue.front();
        mObj->queue.pop();

        if (sEditorEventMeta[(int)event->type].handler)
        {
            sEditorEventMeta[(int)event->type].handler(event, mObj->user);
        }

        mObj->free_event(event);
    }
}

void register_editor_event_handler(EditorEventType type, EditorEventFn handler)
{
    sEditorEventMeta[(int)type].handler = handler;
}

} // namespace LD