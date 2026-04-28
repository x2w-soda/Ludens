#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Serial/Property.h>
#include <Ludens/System/FileSystem.h>
#include <LudensEditor/EditorContext/EditorContextDef.h>

namespace LD {

struct AssetImportInfo;
struct AssetCreateInfo;
struct EditorEvent;

/// @brief User callback to observe editor events.
typedef void (*EditorEventFn)(const EditorEvent* event, void* user);

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

/// @brief Event signaling a request to show a window in modal.
///        Note that this is a request and may be rejected.
struct EditorRequestShowModalEvent : EditorRequestEvent
{
    EditorRequestShowModalEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_SHOW_MODAL)
    {
    }

    EditorWindowType windowType = EDITOR_WINDOW_TYPE_ENUM_COUNT;
    EditorWindowMode windowModeHint = -1;
};

/// @brief Event signaling a request to hide the modal window.
///        Note that this is a request and may be rejected.
struct EditorRequestHideModalEvent : EditorRequestEvent
{
    EditorRequestHideModalEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_HIDE_MODAL)
    {
    }
};

struct EditorRequestWorkspaceLayoutEvent : EditorRequestEvent
{
    EditorRequestWorkspaceLayoutEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_WORKSPACE_LAYOUT)
    {
    }

    EditorUIMainLayout layout = EDITOR_UI_MAIN_LAYOUT_SCENE;
};

/// @brief Event signaling a request to access project settings.
struct EditorRequestProjectSettingsEvent : EditorRequestEvent
{
    EditorRequestProjectSettingsEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS)
    {
    }
};

/// @brief Event signaling that a component in current scene requests changing its script.
struct EditorRequestComponentScriptEvent : EditorRequestEvent
{
    EditorRequestComponentScriptEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_SCRIPT)
    {
    }

    SUID compSUID = 0;
};

/// @brief Event signaling that a component in current scene requests an asset change.
struct EditorRequestComponentAssetEvent : EditorRequestEvent
{
    EditorRequestComponentAssetEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET)
    {
    }

    SUID compSUID = 0;
    AssetID oldAssetID = 0;
    AssetType requestType = ASSET_TYPE_ENUM_COUNT;
    uint32_t assetSlotIndex = 0;
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
struct EditorRequestCreateProjectEvent : EditorRequestEvent
{
    EditorRequestCreateProjectEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_CREATE_PROJECT)
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
struct EditorRequestCreateSceneEvent : EditorRequestEvent
{
    EditorRequestCreateSceneEvent()
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_CREATE_SCENE)
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

struct EditorActionOpenSceneEvent : EditorActionEvent
{
    EditorActionOpenSceneEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_OPEN_SCENE)
    {
    }

    SUID sceneID = 0;
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

struct EditorActionCreateSceneEvent : EditorActionEvent
{
    EditorActionCreateSceneEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_CREATE_SCENE)
    {
    }

    std::string scenePath;
};

/// @brief Synchronously import a batch of assets
struct EditorActionImportAssetsEvent : EditorActionEvent
{
    EditorActionImportAssetsEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS)
    {
    }

    Vector<AssetImportInfo*> batch;
};

/// @brief Begin an async import batch
struct EditorActionImportAssetsAsyncEvent : EditorActionEvent
{
    EditorActionImportAssetsAsyncEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_IMPORT_ASSETS_ASYNC)
    {
    }

    Vector<AssetImportInfo*> batch;
};

struct EditorActionRenameAssetEvent : EditorActionEvent
{
    EditorActionRenameAssetEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_RENAME_ASSET)
    {
    }

    AssetID assetID = 0;
    std::string newPath;
};

struct EditorActionRenameSceneEvent : EditorActionEvent
{
    EditorActionRenameSceneEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_RENAME_SCENE)
    {
    }

    SUID sceneID = 0;
    std::string newPath;
};

struct EditorActionRenameComponentEvent : EditorActionEvent
{
    EditorActionRenameComponentEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_RENAME_COMPONENT)
    {
    }

    SUID compSUID = 0;
    std::string newName;
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

struct EditorActionSetComponentScriptEvent : EditorActionEvent
{
    EditorActionSetComponentScriptEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_SCRIPT)
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
    uint32_t assetSlotIndex = 0;
};

struct EditorActionSetComponentPropsEvent : EditorActionEvent
{
    EditorActionSetComponentPropsEvent()
        : EditorActionEvent(EDITOR_EVENT_TYPE_ACTION_SET_COMPONENT_PROPS)
    {
    }

    SUID compSUID = 0;
    Vector<PropertyDelta> delta;
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
