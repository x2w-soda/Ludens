#pragma once

#include <Ludens/Header/KeyCode.h>
#include <cstdint>

namespace LD {

enum EventType : uint16_t
{
    EVENT_TYPE_WINDOW_CREATE,
    EVENT_TYPE_WINDOW_DESTROY,
    EVENT_TYPE_WINDOW_RESIZE,
    EVENT_TYPE_WINDOW_KEY_DOWN,
    EVENT_TYPE_WINDOW_KEY_UP,
    EVENT_TYPE_WINDOW_MOUSE_MOTION,
    EVENT_TYPE_WINDOW_MOUSE_DOWN,
    EVENT_TYPE_WINDOW_MOUSE_UP,
    EVENT_TYPE_WINDOW_SCROLL,
};

enum EventCategory : uint16_t
{
    EVENT_CATEGORY_WINDOW,
};

struct Event
{
    const EventType type;
    const EventCategory category;

    Event(EventType type, EventCategory category)
        : type(type), category(category) {}
};

} // namespace LD