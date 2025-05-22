#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <optional>

namespace LD {

/// @brief A component for batch rendering 2D primitives.
struct ScreenRenderComponent : Handle<struct ScreenRenderComponentObj>
{
    typedef void (*OnDrawCallback)(ScreenRenderComponent renderer, void* user);

    /// @brief adds the component to render graph
    /// @width screen space render area width
    /// @height screen space render area height
    /// @on_draw user callback to draw 2D primitives
    static ScreenRenderComponent add(RGraph graph, RFormat format, uint32_t width, uint32_t height, OnDrawCallback on_draw, void* user, bool hasSampledImage = false);

    /// @brief get the name of this component
    inline const char* component_name() const { return "screen_render"; }

    /// @brief get the name of the single IO image
    inline const char* io_name() const { return "io"; }

    /// @brief get the name of the optional sampled image
    inline const char* sampled_name() const { return "sampled"; }

    /// @brief if a sampled image is supplied when adding the component, retrieve its image handle during on_draw callback
    RImage get_sampled_image();

    /// @brief draw a rect
    void draw_rect(const Rect& rect, uint32_t color);

    /// @brief draw a 2D image
    /// @param rect image position
    /// @param image a 2D image to be drawn
    void draw_image(const Rect& rect, RImage image);

    /// @brief draw a 2D image with custom uv range
    /// @param rect image position
    /// @param image a 2D image to be drawn
    /// @param uv custom texture uv coordinates
    /// @param color image tint color
    void draw_image_uv(const Rect& rect, RImage image, const Rect& uv, uint32_t color);

    /// @brief draw a single font glyph
    void draw_glyph(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& pos, uint32_t code, uint32_t color);

    /// @brief draw a string of text
    void draw_text(FontAtlas atlas, RImage atlasImage, float fontSize, Vec2& baseline, const char* text, uint32_t color);
};

} // namespace LD