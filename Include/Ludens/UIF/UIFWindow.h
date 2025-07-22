#pragma once

#include <Ludens/UIF/UIFWidget.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UI.h>
#include <vector>

namespace LD {
namespace UIF {

/// @brief UI Window
struct Window : Widget
{
    WidgetNode& node();

    /// @brief set window to position
    void set_pos(const Vec2& pos);

    /// @brief set window to fixed size
    void set_size(const Vec2& size);

    /// @param children 
    void get_children(std::vector<Widget>& children);

    /// @brief get position and size in screen spcae
    Rect get_rect() const;

    std::string get_name() const;

    void show();
    void hide();
    bool is_hidden();
};

struct WindowInfo
{
    const char* name;
};

struct ContextInfo
{
    FontAtlas fontAtlas;   /// default font atlas used to render text
    RImage fontAtlasImage; /// font atlas image handle
};

/// @brief UI framework context.
struct Context : Handle<struct ContextObj>
{
    static Context create(const ContextInfo& info);
    static void destroy(Context ctx);

    /// @brief update UI framework context
    /// @param dt delta time in seconds
    void update(float dt);

    Window add_window(const UILayoutInfo& layoutI, const WindowInfo& windowI);

    /// @brief get window handles
    /// @param windows outputs windows inside the context
    void get_windows(std::vector<Window>& windows);
};

} // namespace UIF
} // namespace LD