#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DataRegistry/DataComponent.h>

namespace LD {

struct EditorContextEvent;

/// @brief User callback to observe editor context events.
typedef void (*EditorContextEventFn)(const EditorContextEvent* event, void* user);

enum EditorContextEventType
{
    EDITOR_CONTEXT_EVENT_PROJECT_LOAD,
    EDITOR_CONTEXT_EVENT_SCENE_LOAD,
    EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION,
    EDITOR_CONTEXT_EVENT_REQUEST_COMPONENT_ASSET,
    EDITOR_CONTEXT_EVENT_REQUEST_NEW_PROJECT,
    EDITOR_CONTEXT_EVENT_REQUEST_OPEN_PROJECT,
    EDITOR_CONTEXT_EVENT_REQUEST_NEW_SCENE,
    EDITOR_CONTEXT_EVENT_REQUEST_OPEN_SCENE,
};

struct EditorContextEvent
{
    const EditorContextEventType type;

    EditorContextEvent(EditorContextEventType type) : type(type) {}
};

/// @brief Event signaling that a Project has been loaded into the editor.
struct EditorContextProjectLoadEvent : EditorContextEvent
{
    EditorContextProjectLoadEvent()
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_PROJECT_LOAD)
    {
    }
};

/// @brief Event signaling that a Scene has been loaded into the editor.
struct EditorContextSceneLoadEvent : EditorContextEvent
{
    EditorContextSceneLoadEvent()
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_SCENE_LOAD)
    {
    }
};

/// @brief Event signaling that the current selected component has changed.
struct EditorContextComponentSelectionEvent : EditorContextEvent
{
    EditorContextComponentSelectionEvent(CUID component)
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION), component(component)
    {
    }

    /// @brief The new component being selected, note that an ID of zero indicates
    ///        that the selection is cleared and no component is being selected in
    ///        the editor.
    const CUID component;
};

/// @brief Event signaling that a component in current scene requests an asset change.
struct EditorContextRequestComponentAssetEvent : EditorContextEvent
{
    EditorContextRequestComponentAssetEvent(CUID component, AUID oldAssetID, AssetType type)
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_REQUEST_COMPONENT_ASSET), component(component), oldAssetID(oldAssetID), requestType(type)
    {
    }

    const CUID component;
    const AUID oldAssetID;
    const AssetType requestType;
};

/// @brief Event signaling the request for creating a new project.
struct EditorContextRequestNewProjectEvent : EditorContextEvent
{
    EditorContextRequestNewProjectEvent()
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_REQUEST_NEW_PROJECT)
    {
    }
};

/// @brief Event signaling the request for opening a project.
struct EditorContextRequestOpenProjectEvent : EditorContextEvent
{
    EditorContextRequestOpenProjectEvent()
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_REQUEST_OPEN_PROJECT)
    {
    }
};

/// @brief Event signaling the request for creating a new scene in current project.
struct EditorContextRequestNewSceneEvent : EditorContextEvent
{
    EditorContextRequestNewSceneEvent()
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_REQUEST_NEW_SCENE)
    {
    }
};

/// @brief Event signaling the request for opening a scene in current project.
struct EditorContextRequestOpenSceneEvent : EditorContextEvent
{
    EditorContextRequestOpenSceneEvent()
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_REQUEST_OPEN_SCENE)
    {
    }
};

} // namespace LD