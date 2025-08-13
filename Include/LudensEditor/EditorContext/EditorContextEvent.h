#pragma once

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
    EditorContextComponentSelectionEvent(DUID component)
        : EditorContextEvent(EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION), component(component)
    {
    }

    /// @brief The new component being selected, note that an ID of zero indicates
    ///        that the selection is cleared and no component is being selected in
    ///        the editor.
    DUID component;
};

} // namespace LD