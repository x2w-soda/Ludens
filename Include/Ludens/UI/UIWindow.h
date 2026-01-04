#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

struct ScreenRenderComponent;

struct UIWindowInfo
{
    const char* name;          /// window identifier
    bool defaultMouseControls; /// allow mouse drag to move and resize window
    bool drawWithScissor;      /// draw child widgets with scissor
    bool hidden;               /// whether the window is created hidden
};

struct UIWindow : UIWidget
{
    UIWindow() = default;
    UIWindow(struct UIWindowObj*);

    /// @brief Perform layout on all widgets in the window.
    void layout();

    /// @brief Render all widgets in the window, if the window is visible.
    /// @param renderer Screen space renderer.
    void render(ScreenRenderComponent& renderer);

    /// @brief set window to position
    void set_pos(const Vec2& pos);

    /// @brief set window to fixed size
    /// @note Does not trigger on_resize callback.
    void set_size(const Vec2& size);

    /// @brief set window to position and fixed size
    void set_rect(const Rect& rect);

    /// @brief Set window background color.
    void set_color(Color bg);

    /// @brief Set color mask for widgets in this window.
    void set_color_mask(Color mask);

    /// @brief get all child widgets in the window
    void get_widgets(Vector<UIWidget>& widgets);

    /// @brief get position and size in screen spcae
    Rect get_rect() const;

    /// @brief Get hash to uniquely identifiy window within its UI context.
    Hash64 get_hash();

    /// @brief Set window resize callback, called when the owning workspace makes adjustments.
    void set_on_resize(void (*onResize)(UIWindow window, const Vec2& size));
};

} // namespace LD