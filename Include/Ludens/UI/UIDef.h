#pragma once

#include <Ludens/Header/Directional.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/Math/Vec2.h>

namespace LD {

enum UIEventType
{
    UI_EVENT_SCROLL,
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_MOUSE_POSITION,
    UI_EVENT_MOUSE_DOWN,
    UI_EVENT_MOUSE_UP,
    UI_EVENT_MOUSE_DRAG,
    UI_EVENT_MOUSE_ENTER,
    UI_EVENT_MOUSE_LEAVE,
    UI_EVENT_FOCUS_ENTER,
    UI_EVENT_FOCUS_LEAVE,
};

struct UIEvent
{
    UIEventType type;

    union
    {
        struct
        {
            KeyMods mods;
            KeyCode code;
        } key;

        struct
        {
            KeyMods mods;
            MouseButton button;
            Vec2 position;
        } mouse;

        struct
        {
            Vec2 offset;
        } scroll;

        struct
        {
            MouseButton button;
            Vec2 position;
            bool begin;
        } drag;
    };
};

enum UIWidgetType
{
    UI_WIDGET_WINDOW = 0,
    UI_WIDGET_SCROLL,
    UI_WIDGET_SCROLL_BAR,
    UI_WIDGET_BUTTON,
    UI_WIDGET_SLIDER,
    UI_WIDGET_TOGGLE,
    UI_WIDGET_PANEL,
    UI_WIDGET_IMAGE,
    UI_WIDGET_TEXT,
    UI_WIDGET_TEXT_EDIT,
    UI_WIDGET_DROPDOWN,
    UI_WIDGET_TYPE_COUNT,
};

enum UIAxis : char
{
    UI_AXIS_X = AXIS_X,
    UI_AXIS_Y = AXIS_Y,
};

enum UIAlign : char
{
    UI_ALIGN_BEGIN,
    UI_ALIGN_CENTER,
    UI_ALIGN_END,
};

enum UISizeType
{
    UI_SIZE_FIXED = 0,
    UI_SIZE_GROW,
    UI_SIZE_WRAP,
    UI_SIZE_FIT,
};

enum UITextEditDomain
{
    UI_TEXT_EDIT_DOMAIN_STRING = 0,
    UI_TEXT_EDIT_DOMAIN_UINT,
    UI_TEXT_EDIT_DOMAIN_F32,
};

} // namespace LD