#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct AssetImportInfo;
struct EditorEvent;

/// @brief User callback to observe editor events.
typedef void (*EditorEventFn)(const EditorEvent* event, void* user);

enum EditorEventType
{
    EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION,
    EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD,
    EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY,
    EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD,
    EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION,
    EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP,
    EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG,
    EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS,
    EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET,
    EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS,
    EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT,
    EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT,
    EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE,
    EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE,
    EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT,
    EDITOR_EVENT_TYPE_REQUEST_DOCUMENT,
    EDITOR_EVENT_TYPE_ACTION_SAVE,
    EDITOR_EVENT_TYPE_ACTION_UNDO,
    EDITOR_EVENT_TYPE_ACTION_REDO,
    EDITOR_EVENT_TYPE_ACTION_NEW_SCENE,
    EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE,
    EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT,
    EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT,
    EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS,
    EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT,
    EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT_SCRIPT,
    EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET,
    EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE,
    EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE,
    EDITOR_EVENT_TYPE_ENUM_COUNT,
};

enum EditorEventCategory
{
    /// @brief Notify events signal a state change that observers may wish to adapt to.
    EDITOR_EVENT_CATEGORY_NOTIFY,

    /// @brief Request events signal that some process should be initiated,
    ///        no actions are committed yet and parameters are often unknown.
    EDITOR_EVENT_CATEGORY_REQUEST,

    /// @brief Action events are transactional and will affect Undo Redo state,
    ///        all parameters for the action type are already known.
    EDITOR_EVENT_CATEGORY_ACTION,
};

struct EditorEvent
{
    const EditorEventType type;
    const EditorEventCategory category;

    EditorEvent(EditorEventType type, EditorEventCategory category)
        : type(type), category(category)
    {
    }
};

/// @brief Broadcast after some action has completed.
struct EditorNotifyEvent : EditorEvent
{
    EditorNotifyEvent() = delete;
    EditorNotifyEvent(EditorEventType type)
        : EditorEvent(type, EDITOR_EVENT_CATEGORY_NOTIFY)
    {
    }
};

/// @brief Broadcast to request some action.
struct EditorRequestEvent : EditorEvent
{
    EditorRequestEvent() = delete;
    EditorRequestEvent(EditorEventType type)
        : EditorEvent(type, EDITOR_EVENT_CATEGORY_REQUEST)
    {
    }
};

struct EditorActionEvent : EditorEvent
{
    EditorActionEvent() = delete;
    EditorActionEvent(EditorEventType type)
        : EditorEvent(type, EDITOR_EVENT_CATEGORY_ACTION)
    {
    }
};

/// @brief Event signaling a result of project creation.
struct EditorNotifyProjectCreationEvent : EditorNotifyEvent
{
    EditorNotifyProjectCreationEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION)
    {
    }

    std::string error = {};      // if not empty, error message of failure
    FS::Path projectSchema = {}; // upon success, the schema of created project
};

/// @brief Event signaling that a Project has been loaded into the editor.
struct EditorNotifyProjectLoadEvent : EditorNotifyEvent
{
    EditorNotifyProjectLoadEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD)
    {
    }
};

struct EditorNotifyProjectSettingsDirtyEvent : EditorNotifyEvent
{
    EditorNotifyProjectSettingsDirtyEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_SETTINGS_DIRTY)
    {
    }

    bool dirtyScreenLayers = false;
};

/// @brief Event signaling that a Scene has been loaded into the editor.
struct EditorNotifySceneLoadEvent : EditorNotifyEvent
{
    EditorNotifySceneLoadEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD)
    {
    }
};

/// @brief Event signaling that the current selected component has changed.
struct EditorNotifyComponentSelectionEvent : EditorNotifyEvent
{
    EditorNotifyComponentSelectionEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION)
    {
    }

    /// @brief The new component being selected, an invalid ID indicates that the selection is cleared.
    CUID cuid = 0;
};

/// @brief Event signaling that files are dragged-and-dropped into editor window.
struct EditorNotifyFileDropEvent : EditorNotifyEvent
{
    EditorNotifyFileDropEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_FILE_DROP)
    {
    }

    Vector<FS::Path> files;
};

/// @brief Event signaling a request to close the current dialog window.
struct EditorRequestCloseDialogEvent : EditorRequestEvent
{
    EditorRequestCloseDialogEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_CLOSE_DIALOG)
    {
    }
};

/// @brief Event signaling a request to access project settings.
struct EditorRequestProjectSettingsEvent : EditorRequestEvent
{
    EditorRequestProjectSettingsEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS)
    {
    }
};

/// @brief Event signaling that a component in current scene requests an asset change.
struct EditorRequestComponentAssetEvent : EditorRequestEvent
{
    EditorRequestComponentAssetEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET)
    {
    }

    SUID component = 0;
    AssetID oldAssetID = 0;
    AssetType requestType = ASSET_TYPE_ENUM_COUNT;
};

/// @brief Event requesting asset import. Import parameters unknown.
struct EditorRequestImportAssetsEvent : EditorRequestEvent
{
    EditorRequestImportAssetsEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS)
    {
    }

    FS::Path srcPath; // source file requesting import
};

/// @brief Event signaling the request for creating a new project.
struct EditorRequestNewProjectEvent : EditorRequestEvent
{
    EditorRequestNewProjectEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT)
    {
    }
};

/// @brief Event signaling the request for opening a project.
struct EditorRequestOpenProjectEvent : EditorRequestEvent
{
    EditorRequestOpenProjectEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT)
    {
    }
};

/// @brief Event signaling the request for creating a new scene in current project.
struct EditorRequestNewSceneEvent : EditorRequestEvent
{
    EditorRequestNewSceneEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE)
    {
    }
};

/// @brief Event signaling the request for opening a scene in current project.
struct EditorRequestOpenSceneEvent : EditorRequestEvent
{
    EditorRequestOpenSceneEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE)
    {
    }
};

/// @brief Event signaling the request for creating a component in current scene.
struct EditorRequestCreateComponentEvent : EditorRequestEvent
{
    EditorRequestCreateComponentEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT)
    {
    }

    SUID parent = 0;
};

/// @brief Event signaling the request for a document.
struct EditorRequestDocumentEvent : EditorRequestEvent
{
    EditorRequestDocumentEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_DOCUMENT)
    {
    }

    // TODO: EditorWindowID as soon as we allow multiple DocumentWindow
    std::string uri;
};

struct EditorActionSaveEvent : EditorActionEvent
{
    EditorActionSaveEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_SAVE)
    {
    }

    bool saveProjectSchema = false;
    bool saveSceneSchema = false;
    bool saveAssetSchema = false;
};

struct EditorActionUndoEvent : EditorActionEvent
{
    EditorActionUndoEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_UNDO)
    {
    }
};

struct EditorActionRedoEvent : EditorActionEvent
{
    EditorActionRedoEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_REDO)
    {
    }
};

struct EditorActionNewSceneEvent : EditorActionEvent
{
    EditorActionNewSceneEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_NEW_SCENE)
    {
    }

    FS::Path newScene;
};

struct EditorActionOpenSceneEvent : EditorActionEvent
{
    EditorActionOpenSceneEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE)
    {
    }

    FS::Path openScene;
};

struct EditorActionOpenProjectEvent : EditorActionEvent
{
    EditorActionOpenProjectEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT)
    {
    }

    FS::Path projectSchema;
};

struct EditorActionCreateProjectEvent : EditorActionEvent
{
    EditorActionCreateProjectEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_CREATE_PROJECT)
    {
    }

    std::string projectName;
    FS::Path projectSchema;
};

struct EditorActionImportAssetsEvent : EditorActionEvent
{
    EditorActionImportAssetsEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS)
    {
    }

    Vector<AssetImportInfo*> batch;
};

struct EditorActionAddComponentEvent : EditorActionEvent
{
    EditorActionAddComponentEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT)
    {
    }

    SUID parentSUID = 0;
    ComponentType compType = COMPONENT_TYPE_ENUM_COUNT;
};

struct EditorActionAddComponentScriptEvent : EditorActionEvent
{
    EditorActionAddComponentScriptEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_ADD_COMPONENT_SCRIPT)
    {
    }

    SUID compSUID = 0;
    AssetID assetID = 0;
};

struct EditorActionSetComponentAssetEvent : EditorActionEvent
{
    EditorActionSetComponentAssetEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_ASSET)
    {
    }

    SUID compSUID = 0;
    AssetID assetID = 0;
};

struct EditorActionCloneComponentSubtreeEvent : EditorActionEvent
{
    EditorActionCloneComponentSubtreeEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_CLONE_COMPONENT_SUBTREE)
    {
    }

    SUID compSUID = 0;
};

struct EditorActionDeleteComponentSubtreeEvent : EditorActionEvent
{
    EditorActionDeleteComponentSubtreeEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_DELETE_COMPONENT_SUBTREE)
    {
    }

    SUID compSUID = 0;
};

} // namespace LD