#pragma once

#include <Ludens/Event/Event.h>
#include <cstdint>

namespace LD {

using WindowID = uint32_t;

struct WindowEvent : Event
{
    const WindowID window;

    WindowEvent(EventType type, WindowID window)
        : Event(type, EVENT_CATEGORY_WINDOW), window(window) {}
};

struct WindowCreateEvent : WindowEvent
{
    WindowCreateEvent(WindowID window)
        : WindowEvent(EVENT_TYPE_WINDOW_CREATE, window) {}
};

struct WindowDestroyEvent : WindowEvent
{
    WindowDestroyEvent(WindowID window)
        : WindowEvent(EVENT_TYPE_WINDOW_DESTROY, window) {}
};

struct WindowResizeEvent : WindowEvent
{
    int32_t width, height;

    WindowResizeEvent(WindowID window, int width, int height)
        : WindowEvent(EVENT_TYPE_WINDOW_RESIZE, window), width(width), height(height) {}
};

struct WindowKeyDownEvent : WindowEvent
{
    const KeyCode key;
    bool repeat;

    WindowKeyDownEvent(WindowID window, KeyCode key, bool repeat)
        : WindowEvent(EVENT_TYPE_WINDOW_KEY_DOWN, window), key(key), repeat(repeat) {}
};

struct WindowKeyUpEvent : WindowEvent
{
    const KeyCode key;

    WindowKeyUpEvent(WindowID window, KeyCode key)
        : WindowEvent(EVENT_TYPE_WINDOW_KEY_UP, window), key(key) {}
};

struct WindowMouseMotionEvent : WindowEvent
{
    float xpos, ypos;

    WindowMouseMotionEvent(WindowID window, float x, float y)
        : WindowEvent(EVENT_TYPE_WINDOW_MOUSE_MOTION, window), xpos(x), ypos(y) {}
};

struct WindowMouseDownEvent : WindowEvent
{
    const MouseButton button;

    WindowMouseDownEvent(WindowID window, MouseButton btn)
        : WindowEvent(EVENT_TYPE_WINDOW_MOUSE_DOWN, window), button(btn) {}
};

struct WindowMouseUpEvent : WindowEvent
{
    const MouseButton button;

    WindowMouseUpEvent(WindowID window, MouseButton btn)
        : WindowEvent(EVENT_TYPE_WINDOW_MOUSE_UP, window), button(btn) {}
};

struct WindowScrollEvent : WindowEvent
{
    float xoffset, yoffset;

    WindowScrollEvent(WindowID window, float xoffset, float yoffset)
        : WindowEvent(EVENT_TYPE_WINDOW_SCROLL, window), xoffset(xoffset), yoffset(yoffset) {}
};

} // namespace LD