#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DataRegistry/DataComponent.h>

namespace LD {

struct EditorEvent;

/// @brief User callback to observe editor events.
typedef void (*EditorEventFn)(const EditorEvent* event, void* user);

enum EditorEventType
{
    EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD,
    EDITOR_EVENT_TYPE_NOTIFY_SCENE_LOAD,
    EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION,
    EDITOR_EVENT_TYPE_REQUEST_PROJECT_SETTINGS,
    EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET,
    EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT,
    EDITOR_EVENT_TYPE_REQUEST_OPEN_PROJECT,
    EDITOR_EVENT_TYPE_REQUEST_NEW_SCENE,
    EDITOR_EVENT_TYPE_REQUEST_OPEN_SCENE,
    EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT,
};

enum EditorEventCategory
{
    EDITOR_EVENT_CATEGORY_NOTIFY,
    EDITOR_EVENT_CATEGORY_REQUEST,
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

/// @brief Event signaling that a Project has been loaded into the editor.
struct EditorNotifyProjectLoadEvent : EditorNotifyEvent
{
    EditorNotifyProjectLoadEvent()
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD)
    {
    }
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
    EditorNotifyComponentSelectionEvent(CUID component)
        : EditorNotifyEvent(EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION), component(component)
    {
    }

    /// @brief The new component being selected, an ID of zero indicates that the selection is cleared.
    const CUID component;
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
    EditorRequestComponentAssetEvent(CUID component, AUID oldAssetID, AssetType type)
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_COMPONENT_ASSET), component(component), oldAssetID(oldAssetID), requestType(type)
    {
    }

    const CUID component;
    const AUID oldAssetID;
    const AssetType requestType;
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
    EditorRequestCreateComponentEvent(CUID parent)
        : EditorRequestEvent(EDITOR_EVENT_TYPE_REQUEST_CREATE_COMPONENT), parent(parent)
    {
    }

    const CUID parent;
};

} // namespace LD