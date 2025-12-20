#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <optional>

namespace LD {

struct ScreenRenderComponentInfo;

/// @brief A component for batch rendering 2D primitives.
struct ScreenRenderComponent : Handle<struct ScreenRenderComponentObj>
{
    /// @brief Adds the component to render graph.
    static ScreenRenderComponent add(RGraph graph, const ScreenRenderComponentInfo& info);

    /// @brief get the name of this component instance
    const char* component_name() const;

    /// @brief get the name of the single IO image
    inline const char* io_name() const { return "io"; }

    /// @brief get the name of the optional sampled image
    inline const char* sampled_name() const { return "sampled"; }

    /// @brief if a sampled image is supplied when adding the component, retrieve its image handle during on_draw callback
    RImage get_sampled_image();

    /// @brief Get the screen extent of the component this frame
    void get_screen_extent(uint32_t& screenWidth, uint32_t& screenHeight);

    /// @brief Push a scissor rect onto stack, only the top scissor takes effect.
    /// @note This forces a flush of the current batch since the scissor state changes.
    void push_scissor(const Rect& scissor);

    /// @brief Pop a scissor rect off the stack, the remaining top scissor takes effect.
    /// @note This forces a flush of the current batch since the scissor state changes.
    void pop_scissor();

    /// @brief Push a color mask onto stack, only the top color mask takes effect.
    ///        Subsequent draw calls will have their colors multiplied with the mask.
    void push_color_mask(Color mask);

    /// @brief Pop a color mask off the stack, the remaining top color mask takes effect.
    void pop_color_mask();

    /// @brief Draw a 2D primitive.
    void draw(const Vec2& tl, const Vec2& tr, const Vec2& br, const Vec2& bl, RImage image, Color color);

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
    /// @param color Tint color.
    void draw_image(const Rect& rect, RImage image, Color color);

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

struct ScreenRenderComponentInfo
{
    const char* name;                                                   /// component instance name, distinct among instances of this component type
    RFormat format;                                                     /// color format
    bool hasSampledImage;                                               /// whether to use the optional input sampled image
    bool hasInputImage;                                                 /// whether to draw on top of existing image, or to generate a new one
    Color clearColor;                                                   /// if hasInputImage is false, the clear color used to initialize the output
    void (*onDrawCallback)(ScreenRenderComponent renderer, void* user); /// invoked at draw time
    const Vec2* screenExtent;                                           /// if not null, replaces the default render graph screen extent
    void* user;                                                         /// component instance user
};

} // namespace LD