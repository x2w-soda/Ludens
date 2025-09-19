#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

/// @brief Callback when an option in the dropdown menu is selected.
typedef bool (*UIDropdownWindowCallback)(int option, const Rect& optionRect, void* user);

struct UIDropdownWindowInfo
{
    UIContext context;
    EditorTheme theme;
    UIDropdownWindowCallback callback;
    void* user;
};

/// @brief A dropdown window holding options.
struct UIDropdownWindow : Handle<struct UIDropdownWindowObj>
{
    /// @brief Create dropdown window widget.
    static UIDropdownWindow create(const UIDropdownWindowInfo& info);

    /// @brief Destroy dropdown window widget.
    static void destroy(UIDropdownWindow dropdown);

    /// @brief Add an option to the dropdown window.
    void add_option(const char* text, int optionIndex);

    /// @brief Get window handle.
    UIWindow get_native();

    /// @brief Set option callback after creation.
    void set_callback(UIDropdownWindowCallback cb);

    /// @brief Show the dropdown window.
    void show();

    /// @brief Hide the dropdown window.
    void hide();
};

} // namespace LD