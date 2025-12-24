#pragma once

#include <Ludens/Event/Event.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWindow.h>
#include <cstdint>
#include <vector>

namespace LD {

struct ScreenRenderComponent;

struct UIContextInfo
{
    FontAtlas fontAtlas;   /// default font atlas used to render text
    RImage fontAtlasImage; /// font atlas image handle
    UITheme theme;         /// the UI theme to use for widgets in this context
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

    /// @brief Render all windows and widgets in a layer.
    /// @param renderer The screen space renderer to draw the layer with.
    void render_layer(Hash32 layer, ScreenRenderComponent& renderer);

    /// @brief Pass an application event to the UI context.
    bool forward_event(const Event* event);

    /// @brief update mouse cursor position in context
    void input_mouse_position(const Vec2& pos);

    /// @brief notify that a mouse button has been pressed
    void input_mouse_down(MouseButton btn);

    /// @brief notify that a mouse button has been released
    void input_mouse_up(MouseButton btn);

    /// @brief Notify that a key has been pressed.
    void input_key_down(KeyCode key);

    /// @brief Notify that a key has been released.
    void input_key_up(KeyCode key);

    /// @brief Notify that the mouse wheel or touchpad has been scrolled.
    /// @param offset A standard mouse wheel scroll provides offset along Y axis.
    void input_scroll(const Vec2& offset);

    /// @brief Perform layout on all widgets, across all windows, across all layers.
    void layout();

    /// @brief Add a layer to context.
    void add_layer(Hash32 layer);

    /// @brief Remove a layer from context. This removes all windows within the layer.
    void remove_layer(Hash32 layer);

    /// @brief Raise layer to top in context.
    void raise_layer(Hash32 layer);

    /// @brief Check if layer exists in context.
    bool has_layer(Hash32 layer);

    /// @brief Get all layers in draw order (last layer in vector receives input first).
    void get_layers(std::vector<Hash32>& layers);

    /// @brief add a window to the context
    UIWindow add_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);

    /// @brief Remove a window from the context.
    void remove_window(UIWindow window);

    /// @brief get window handles
    /// @param windows outputs windows inside the context
    void get_windows(Hash32 layer, std::vector<UIWindow>& windows);

    /// @brief Get current UI theme, shared by all widgets in this context.
    UITheme get_theme();

    /// @brief Get mouse cursor position.
    Vec2 get_mouse_pos();
};

} // namespace LD