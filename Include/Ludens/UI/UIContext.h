#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UILayer.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/WindowRegistry/WindowRegistryDef.h>

#include <cstdint>

namespace LD {

struct WindowEvent;

struct UIContextInfo
{
    UIFont font;     /// default font for text rendering, this font out-lives the UIContext
    UIFont fontMono; /// optional font for monospace text
    UITheme theme;   /// the UI theme to use for widgets in this context
    void (*onEvent)(UIWidget widget, const UIEvent& event, void* user);
    void* user;
};

/// @brief A UI context is a host for UI elements to be placed on an imaginary 2D grid.
///        UI elements do not communicate across contexts.
struct UIContext : Handle<struct UIContextObj>
{
    /// @brief create a UI context.
    /// @return a UI context handle
    static UIContext create(const UIContextInfo& info);

    /// @brief destroy a UI context.
    static void destroy(UIContext ctx);

    /// @brief update UI context
    /// @param delta delta time in seconds
    void update(float delta);

    /// @brief Render each UILayer in order.
    void render(ScreenRenderComponent renderer);

    bool input_scroll(const Vec2& offset);
    bool input_key_down(KeyCode code, KeyMods mods);
    bool input_key_up(KeyCode code, KeyMods mods);
    bool input_mouse_position(const Vec2& pos);
    bool input_mouse_down(MouseButton btn, KeyMods mods, const Vec2& pos);
    bool input_mouse_up(MouseButton btn, KeyMods mods, const Vec2& pos);

    /// @brief Translate a Window event to input and pass it to the UI context,
    ///        the WindowID is ignored and mouse positions are relative to UI context origin.
    bool input_window_event(const WindowEvent* event);

    /// @brief Create and add a layer to context.
    UILayer create_layer(const char* layerName);

    /// @brief Destroy a layer in context. This destroys all workspaces within the layer.
    void destroy_layer(UILayer layer);

    /// @brief Get all layers in draw order (last layer in vector receives input first).
    void get_layers(Vector<UILayer>& layers);

    /// @brief Get current UI theme, shared by all widgets in this context.
    UITheme get_theme();

    /// @brief Get mouse cursor position.
    Vec2 get_mouse_pos();

    /// @brief Get cursor type suggestion, calculated from focus state and hovered widgets.
    CursorType get_cursor_hint();

    void set_user(void* user);
    void set_on_event(void (*onEvent)(UIWidget widget, const UIEvent& event, void* user));

    std::string print();
};

} // namespace LD