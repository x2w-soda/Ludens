#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIWidget.h>
#include <vector>

namespace LD {

struct UIWindowInfo
{
    const char* name;          /// window identifier
    bool defaultMouseControls; /// allow mouse drag to move and resize window
    bool drawWithScissor;      /// draw child widgets with scissor
    bool hidden;               /// whether the window is created hidden
};

struct UIWindow : UIWidget
{
    /// @brief Raise the window to top.
    void raise();

    /// @brief If window is visible, draw all widgets within.
    /// @param renderer Screen space renderer
    void draw(ScreenRenderComponent renderer);

    /// @brief Force a layout invalidation.
    void layout();

    /// @brief check whether the window responds to user input and should be drawn
    bool is_hidden();

    /// @brief set window to position
    void set_pos(const Vec2& pos);

    /// @brief set window to fixed size
    void set_size(const Vec2& size);

    /// @brief set window to position and fixed size
    void set_rect(const Rect& rect);

    /// @brief get all child widgets in the window
    void get_widgets(std::vector<UIWidget>& widgets);

    /// @brief get position and size in screen spcae
    Rect get_rect() const;

    /// @brief get window name identifier
    std::string get_name() const;
};

} // namespace LD