#pragma once

#include <Ludens/Header/Color.h>
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
    /// @on_draw user callback to draw 2D primitives
    static ScreenRenderComponent add(RGraph graph, RFormat format, OnDrawCallback on_draw, void* user, bool hasSampledImage = false, bool isOutputImage = false);

    /// @brief get the name of this component
    inline const char* component_name() const { return "screen_render"; }

    /// @brief get the name of the single IO image
    inline const char* io_name() const { return "io"; }

    /// @brief get the name of the optional sampled image
    inline const char* sampled_name() const { return "sampled"; }

    /// @brief if a sampled image is supplied when adding the component, retrieve its image handle during on_draw callback
    RImage get_sampled_image();

    /// @brief draw a rect
    /// @param rect render area in screen space
    /// @param color rect fill color
    void draw_rect(const Rect& rect, Color color);

    /// @brief draw a rect outline
    /// @param rect render area in screen space
    /// @param border outline thickness, grows inwards within render area
    /// @param color rect outline color
    void draw_rect_outline(const Rect& rect, float border, Color color);

    /// @brief draw a 2D image
    /// @param rect image position
    /// @param image a 2D image to be drawn
    void draw_image(const Rect& rect, RImage image);

    /// @brief draw a 2D image with custom uv range
    /// @param rect image position
    /// @param image a 2D image to be drawn
    /// @param uv custom texture uv coordinates
    /// @param color image tint color
    void draw_image_uv(const Rect& rect, RImage image, const Rect& uv, Color color);

    /// @brief draw a single font glyph using top-left corner origin
    /// @param atlas font atlas
    /// @param atlasImage corresponding font atlas image
    /// @param fontSize font render size
    /// @param pos top left origin of the glyph to be rendered
    /// @param code unicode codepoint
    /// @param color glyph fill color
    void draw_glyph(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& pos, uint32_t code, Color color);

    /// @brief draw a single font glyph using position on baseline
    /// @param atlas font atlas
    /// @param atlasImage corresponding font atlas image
    /// @param fontSize font render size
    /// @param baseline cursor position on a horizontal baseline
    /// @param code unicode codepoint
    /// @param color glyph fill color
    /// @return the advanceX offset that can be applied to baseline x position.
    float draw_glyph_baseline(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& baseline, uint32_t code, Color color);

    /// @brief draw a string of text
    void draw_text(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& pos, const char* text, Color color, float wrapWidth = 0.0f);
};

} // namespace LD