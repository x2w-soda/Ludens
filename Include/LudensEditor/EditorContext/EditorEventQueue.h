#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditorEvent.h>

namespace LD {

void register_editor_event_handler(EditorEventType type, EditorEventFn handler);

/// @brief A queue of editor events to process sequentially.
struct EditorEventQueue : Handle<struct EditorEventQueueObj>
{
    /// @brief Create the event queue.
    static EditorEventQueue create(void* user);

    /// @brief Destroy the event queue and all allocated EditorEvents.
    static void destroy(EditorEventQueue queue);

    /// @brief Allocate an event in queue for later processing.
    /// @return An editor event with requested type, this is owned by queue.
    EditorEvent* enqueue(EditorEventType type);

    /// @brief Process all events sequentially and synchronously until queue is empty.
    void poll_events();
};

} // namespace LD