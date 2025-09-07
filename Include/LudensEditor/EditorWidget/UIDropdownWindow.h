#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

typedef void (*UIDropdownWindowCallback)(int option, const Rect& optionRect, void* user);

struct UIDropdownWindowInfo
{
    UIContext context;
    EditorTheme theme;
    UIDropdownWindowCallback callback;
    void* user;
};

struct UIDropdownWindow : Handle<struct UIDropdownWindowObj>
{
    static UIDropdownWindow create(const UIDropdownWindowInfo& info);
    static void destroy(UIDropdownWindow dropdown);

    /// @brief Add an option to the dropdown window.
    void add_option(const char* text);

    /// @brief Get window handle.
    UIWindow get_native();

    /// @brief Set option callback after creation.
    void set_callback(UIDropdownWindowCallback cb);
};

} // namespace LD