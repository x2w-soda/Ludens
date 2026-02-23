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
    const KeyCode code;
    const KeyMods mods;
    bool repeat;

    WindowKeyDownEvent(WindowID window, KeyCode code, KeyMods mods = 0, bool repeat = false)
        : WindowEvent(EVENT_TYPE_WINDOW_KEY_DOWN, window), code(code), mods(mods), repeat(repeat) {}
};

struct WindowKeyUpEvent : WindowEvent
{
    const KeyCode code;
    const KeyMods mods;

    WindowKeyUpEvent(WindowID window, KeyCode code, KeyMods mods = 0)
        : WindowEvent(EVENT_TYPE_WINDOW_KEY_UP, window), code(code), mods(mods) {}
};

struct WindowMousePositionEvent : WindowEvent
{
    float xpos, ypos;

    WindowMousePositionEvent(WindowID window, float x, float y)
        : WindowEvent(EVENT_TYPE_WINDOW_MOUSE_POSITION, window), xpos(x), ypos(y) {}
};

struct WindowMouseDownEvent : WindowEvent
{
    const MouseButton button;
    const KeyMods mods;

    WindowMouseDownEvent(WindowID window, MouseButton btn, KeyMods mods = 0)
        : WindowEvent(EVENT_TYPE_WINDOW_MOUSE_DOWN, window), button(btn), mods(mods) {}
};

struct WindowMouseUpEvent : WindowEvent
{
    const MouseButton button;
    const KeyMods mods;

    WindowMouseUpEvent(WindowID window, MouseButton btn, KeyMods mods = 0)
        : WindowEvent(EVENT_TYPE_WINDOW_MOUSE_UP, window), button(btn), mods(mods) {}
};

struct WindowScrollEvent : WindowEvent
{
    float xoffset, yoffset;

    WindowScrollEvent(WindowID window, float xoffset, float yoffset)
        : WindowEvent(EVENT_TYPE_WINDOW_SCROLL, window), xoffset(xoffset), yoffset(yoffset) {}
};

} // namespace LD