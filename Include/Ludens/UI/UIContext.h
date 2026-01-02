#pragma once

#include <Ludens/Event/Event.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UILayer.h>

#include <cstdint>

namespace LD {

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

    /// @brief Pass an application event to the UI context.
    bool on_event(const Event* event);

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
};

} // namespace LD