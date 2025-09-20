#pragma once

#include <Ludens/Header/KeyCode.h>

namespace LD {

enum EventType
{
    EVENT_TYPE_APPLICAITON_RESIZE,
    EVENT_TYPE_KEY_DOWN,
    EVENT_TYPE_KEY_UP,
    EVENT_TYPE_MOUSE_MOTION,
    EVENT_TYPE_MOUSE_DOWN,
    EVENT_TYPE_MOUSE_UP,
    EVENT_TYPE_SCROLL,
};

struct Event
{
    const EventType type;

    Event(EventType type) : type(type) {}
};

struct ApplicationResizeEvent : Event
{
    int width, height;

    ApplicationResizeEvent(int width, int height) : Event(EVENT_TYPE_APPLICAITON_RESIZE), width(width), height(height) {}
};

struct KeyDownEvent : Event
{
    const KeyCode key;
    bool repeat;

    KeyDownEvent(KeyCode key, bool repeat) : Event(EVENT_TYPE_KEY_DOWN), key(key), repeat(repeat) {}
};

struct KeyUpEvent : Event
{
    const KeyCode key;

    KeyUpEvent(KeyCode key) : Event(EVENT_TYPE_KEY_UP), key(key) {}
};

struct MouseMotionEvent : Event
{
    float xpos, ypos;

    MouseMotionEvent(float x, float y) : Event(EVENT_TYPE_MOUSE_MOTION), xpos(x), ypos(y) {}
};

struct MouseDownEvent : Event
{
    const MouseButton button;

    MouseDownEvent(MouseButton btn) : Event(EVENT_TYPE_MOUSE_DOWN), button(btn) {}
};

struct MouseUpEvent : Event
{
    const MouseButton button;

    MouseUpEvent(MouseButton btn) : Event(EVENT_TYPE_MOUSE_UP), button(btn) {}
};

struct ScrollEvent : Event
{
    float xoffset, yoffset;

    ScrollEvent(float xoffset, float yoffset) : Event(EVENT_TYPE_SCROLL), xoffset(xoffset), yoffset(yoffset) {}
};

} // namespace LD