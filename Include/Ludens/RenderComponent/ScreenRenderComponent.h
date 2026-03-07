#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderComponent/Pipeline/QuadPipeline.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

enum QuadPipelineType;
struct ScreenRenderComponentInfo;
struct QuadVertex;

/// @brief A component for batch rendering 2D primitives with QuadPipelines.
///        1. bind_quad_pipeline
///        2. set_view_projection_index to select the view projection matrices
///        3. optionally configure viewport scissor state
///        4. issue draw calls that are supported by the QuadPipeline variant
struct ScreenRenderComponent : Handle<struct ScreenRenderComponentObj>
{
    /// @brief Adds the component to render graph.
    static ScreenRenderComponent add(RGraph graph, const ScreenRenderComponentInfo& info);

    /// @brief get the name of this component instance
    const char* component_name() const;

    RGraphImage color_attachment();
    RGraphImage sampled_attachment();

    /// @brief if a sampled image is supplied when adding the component, retrieve its image handle during on_draw callback
    RImage get_sampled_image();

    /// @brief Get the screen extent of the component this frame
    void get_screen_extent(uint32_t& screenWidth, uint32_t& screenHeight);

    /// @brief Set the index for retrieving view projection matrices.
    /// @note This forces a flush of the current batch.
    void set_view_projection_index(int vpIndex);

    /// @brief Push a viewport screen rect onto stack, only the top viewport takes effect.
    /// @note This forces a flush of the current batch since the viewport state changes.
    void push_viewport(const Rect& viewport);

    /// @brief Push a viewport normalized rect onto stack, only the top viewport takes effect.
    /// @note This forces a flush of the current batch since the viewport state changes.
    void push_viewport_normalized(const Rect& viewport);

    /// @brief Pop a viewport rect off the stack, the remaining top viewport takes effect.
    /// @note This forces a flush of the current batch since the viewport state changes.
    void pop_viewport();

    /// @brief Push a scissor screen rect onto stack, only the top scissor takes effect.
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

    /// @note This forces a flush of the current batch since the pipeline changes.
    void bind_quad_pipeline(QuadPipelineType type);

    /// @brief Get address to write 4 rect vertices. QuadVertex::control bits are already decided by image.
    QuadVertex* draw(RImage image);

    /// @brief draw a rect
    /// @param rect render area in screen space
    /// @param color rect fill color
    void draw_rect(const Rect& rect, Color color);

    /// @brief Draw a rect.
    /// @param model Model matrix
    /// @param localRect Local rect dimensions
    /// @param color Rect fill color
    void draw_rect(const Mat4& model, const Rect& localRect, Color color);

    /// @brief draw a rect outline
    /// @param rect render area in screen space
    /// @param color rect outline color
    /// @param thickness outline thickness, grows inwards within render area
    void draw_rect_outline(const Rect& rect, Color color, float thickness);

    /// @brief Draw a rect outline.
    /// @param model Model matrix.
    /// @param localRect Local rect dimensions.
    /// @param color Rect outline color.
    /// @param thickness Outline thickness, grows inwards within render area.
    void draw_rect_outline(const Mat4& model, const Rect& localRect, Color color, float thickness);

    /// @brief Draw a rect with border radius.
    /// @param rect Render area in screen space.
    /// @param color Fill color.
    /// @param radius Normalized border radius ratio.
    void draw_rect_rounded(const Rect& rect, Color color, float radius);

    /// @brief Draw an ellipse bound in a rect.
    /// @param rect Render area in screen space.
    /// @param color Ellipse color.
    void draw_ellipse(const Rect& rect, Color color);

    /// @brief Draw an ellipse sampling from image.
    /// @param rect Render area in screen space.
    /// @param color Ellipse tint color.
    /// @param image The image to be drawn.
    void draw_ellipse_image(const Rect& rect, Color color, RImage image, const Rect& uv);

    /// @brief Draw a 2D image
    /// @param rect image position
    /// @param color Tint color.
    /// @param image a 2D image to be drawn
    /// @param forceAlphaOne Whether to override image alpha channel with 1.
    void draw_image(const Rect& rect, Color color, RImage image, const Rect& uv, bool forceAlphaOne);

    /// @brief Draw a 2D image with rounded border.
    /// @param rect Render area in screen space.
    /// @param image 2D image handle.
    /// @param uv Image UV area.
    /// @param color Image tint color.
    /// @param radius Normalized border radius ratio.
    void draw_image_rounded(const Rect& rect, Color color, RImage image, const Rect& uv, float radius);

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