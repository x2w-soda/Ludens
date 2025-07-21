#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/ScreenRender.h>
#include <Ludens/System/Memory.h>
#include <array>
#include <vector>

namespace LD {

// clang-format off
static const char sRectVS[] = R"(
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in uint aColor;
layout (location = 3) in uint aControl;

layout (location = 0) out vec2 vUV;
layout (location = 1) out flat uint vColor;
layout (location = 2) out flat uint vControl;
)"
LD_GLSL_FRAME_SET
R"(

void main()
{
    float ndcx = (aPos.x / uFrame.screenExtent.x) * 2.0 - 1.0;
    float ndcy = (aPos.y / uFrame.screenExtent.y) * 2.0 - 1.0;
    gl_Position = vec4(ndcx, ndcy, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
    vControl = aControl;
}
)";

static const char sRectFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 1) in flat uint vColor;
layout (location = 2) in flat uint vControl;
layout (location = 0) out vec4 fColor;

layout (set = 1, binding = 0) uniform sampler2D uImages[8];

void main()
{
    vec4 imageColor = vec4(1.0);

    uint imageIdx = vControl & 15;
    uint imageHintBits = (vControl >> 4) & 15;
    uint filterRatioBits = (vControl >> 8) & 255;
    float filterRatio = float(filterRatioBits) / 8.0f;

    switch (imageIdx)
    {
        case 0: break;
        case 1: imageColor = texture(uImages[0], vUV); break;
        case 2: imageColor = texture(uImages[1], vUV); break;
        case 3: imageColor = texture(uImages[2], vUV); break;
        case 4: imageColor = texture(uImages[3], vUV); break;
        case 5: imageColor = texture(uImages[4], vUV); break;
        case 6: imageColor = texture(uImages[5], vUV); break;
        case 7: imageColor = texture(uImages[6], vUV); break;
        case 8: imageColor = texture(uImages[7], vUV); break;
    }

    float r = float((vColor >> 24) & 0xFF) / 255.0f;
    float g = float((vColor >> 16) & 0xFF) / 255.0f;
    float b = float((vColor >> 8) & 0xFF) / 255.0f;
    float a = float(vColor & 0xFF) / 255.0f;
    vec4 tint = vec4(r, g, b, a);

    float screenPxRange = 2.0 * filterRatio;
    float sd = imageColor.r;
    float screenPxDistance = screenPxRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    vec4 color = imageColor * tint;

    switch (imageHintBits)
    {
        case 1: // single channel font bitmap
            color = tint * vec4(imageColor.r);
            break;
        case 2: // font SDF
            color = mix(vec4(0.0), tint, opacity);
            break;
    }

    fColor = color;
}
)";
// clang-format on

constexpr uint32_t sMaxRectCount = 1024;
constexpr uint32_t sMaxRectVertexCount = sMaxRectCount * 4;
constexpr uint32_t sMaxRectIndexCount = sMaxRectCount * 6;

struct ScreenRenderComponentObj
{
    /// @brief for host mapped memory, we need duplicates per frame in flight
    struct Frame
    {
        std::vector<RBuffer> rectVBOs;
        RSet screenSet;
        bool isScreenSetDirty;
    };

    RDevice device;
    RShader rectVS;
    RShader rectFS;
    RBuffer rectIBO;
    RPipeline rectPipeline;
    RPipelineLayoutInfo screenPipelineLayout;
    RCommandList list;
    RSetPool setPool;
    RImage whitePixel;
    RImage imageSlots[8];
    RectVertexBatch<sMaxRectCount> rectBatch;
    RGraphicsPass graphicsPass;
    uint32_t imageCounter;
    uint32_t batchIdx;
    uint32_t frameIdx;
    ScreenRenderComponent::OnDrawCallback on_draw;
    std::vector<Frame> frames;
    void* user;
    bool hasInit;
    bool hasSampledImage;

    void init(RDevice device);

    void flush_rects();

    int get_image_index(RImage image);

    static void on_release(void* user);
    static void on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData);

} sSRCompObj; // TODO: non-singleton

void ScreenRenderComponentObj::init(RDevice device)
{
    if (hasInit)
        return;

    hasInit = true;

    this->device = device;
    frames.resize(device.get_frames_in_flight_count());
    uint32_t* indices = (uint32_t*)heap_malloc(sizeof(uint32_t) * sMaxRectIndexCount, MEMORY_USAGE_RENDER);
    rectBatch.write_indices(indices);
    batchIdx = 0;
    imageCounter = 0;
    list = {};

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT,
        .size = sizeof(uint32_t) * sMaxRectIndexCount,
        .hostVisible = false,
    };
    rectIBO = device.create_buffer(bufferI);

    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    stager.add_buffer_data(rectIBO, indices);
    heap_free(indices);

    rectVS = device.create_shader({.type = RSHADER_TYPE_VERTEX, .glsl = sRectVS});
    rectFS = device.create_shader({.type = RSHADER_TYPE_FRAGMENT, .glsl = sRectFS});

    static RSetBindingInfo setBinding = {0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 8};
    static RSetLayoutInfo screenSetLayout = {.bindingCount = 1, .bindings = &setBinding};
    static RSetLayoutInfo setLayouts[2];
    setLayouts[0] = sFrameSetLayout;
    setLayouts[1] = screenSetLayout;

    screenPipelineLayout.setLayoutCount = 2;
    screenPipelineLayout.setLayouts = setLayouts;

    RPipelineBlendState blendState = RUtil::make_default_blend_state();

    std::array<RShader, 2> shaders{rectVS, rectFS};
    std::vector<RVertexAttribute> attrs;
    RVertexBinding binding = {.inputRate = RBINDING_INPUT_RATE_VERTEX, .stride = sizeof(RectVertex)};
    get_rect_vertex_attributes(attrs);

    RPipelineInfo pipelineI{
        .shaderCount = (uint32_t)shaders.size(),
        .shaders = shaders.data(),
        .vertexAttributeCount = (uint32_t)attrs.size(),
        .vertexAttributes = attrs.data(),
        .vertexBindingCount = 1,
        .vertexBindings = &binding,
        .layout = screenPipelineLayout,
        .depthStencil = {
            .depthTestEnabled = false,
        },
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendState,
        },
    };

    rectPipeline = device.create_pipeline(pipelineI);

    setPool = device.create_set_pool({
        .layout = screenSetLayout,
        .maxSets = device.get_frames_in_flight_count(),
    });

    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, 1, 1, {});
    whitePixel = device.create_image(imageI);
    uint32_t pixel = 0xFFFFFFFF;
    stager.add_image_data(whitePixel, &pixel, RIMAGE_LAYOUT_SHADER_READ_ONLY);

    stager.submit(device.get_graphics_queue());

    RImageLayout layouts[8];
    std::fill(layouts, layouts + 8, RIMAGE_LAYOUT_SHADER_READ_ONLY);
    std::fill(imageSlots, imageSlots + 8, whitePixel);

    for (Frame& frame : frames)
    {
        bufferI = {
            .usage = RBUFFER_USAGE_VERTEX_BIT,
            .size = sizeof(RectVertex) * sMaxRectVertexCount,
            .hostVisible = true, // persistent mapping
        };
        frame.rectVBOs = {device.create_buffer(bufferI)};
        frame.rectVBOs[0].map();

        frame.screenSet = setPool.allocate();
        frame.isScreenSetDirty = false;

        RSetImageUpdateInfo updateI;
        updateI.set = frame.screenSet;
        updateI.dstBinding = 0;
        updateI.dstArrayIndex = 0;
        updateI.imageCount = 8;
        updateI.imageLayouts = layouts;
        updateI.imageBindingType = RBINDING_TYPE_COMBINED_IMAGE_SAMPLER;
        updateI.images = imageSlots;
        device.update_set_images(1, &updateI);
    }

    RGraph::add_release_callback(this, &ScreenRenderComponentObj::on_release);
}

void ScreenRenderComponentObj::flush_rects()
{
    LD_PROFILE_SCOPE;

    Frame& frame = frames[frameIdx];

    uint32_t rectCount = rectBatch.get_rect_count();
    uint32_t vertexCount;
    RectVertex* vertices = rectBatch.get_vertices(vertexCount);
    frame.rectVBOs[batchIdx].map_write(0, sizeof(RectVertex) * vertexCount, vertices);

    rectBatch.reset();

    RImageLayout layouts[8];
    std::fill(layouts, layouts + 8, RIMAGE_LAYOUT_SHADER_READ_ONLY);

    if (frame.isScreenSetDirty)
    {
        LD_PROFILE_SCOPE_NAME("update set images");

        frame.isScreenSetDirty = false;

        RSetImageUpdateInfo updateI;
        updateI.set = frame.screenSet;
        updateI.dstBinding = 0;
        updateI.dstArrayIndex = 0;
        updateI.imageCount = imageCounter;
        updateI.imageLayouts = layouts;
        updateI.imageBindingType = RBINDING_TYPE_COMBINED_IMAGE_SAMPLER;
        updateI.images = imageSlots;
        device.update_set_images(1, &updateI);
    }

    list.cmd_bind_vertex_buffers(0, 1, frame.rectVBOs.data() + batchIdx);
    list.cmd_bind_graphics_sets(screenPipelineLayout, 1, 1, &frame.screenSet);

    RDrawIndexedInfo drawI = {
        .indexCount = rectCount * 6,
        .indexStart = 0,
        .instanceCount = 1,
        .instanceStart = 0,
    };
    list.cmd_draw_indexed(drawI);

    if (++batchIdx < frame.rectVBOs.size())
        return;

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_VERTEX_BIT,
        .size = sizeof(RectVertex) * sMaxRectVertexCount,
        .hostVisible = true, // persistent mapping
    };

    frame.rectVBOs.push_back(device.create_buffer(bufferI));
    frame.rectVBOs.back().map();
}

int ScreenRenderComponentObj::get_image_index(RImage image)
{
    for (int i = 0; i < imageCounter; i++)
    {
        if (imageSlots[i] == image)
            return i + 1;
    }

    if (imageCounter == 8)
        return -1; // caller should flush

    frames[frameIdx].isScreenSetDirty = true;
    imageSlots[imageCounter++] = image;

    return imageCounter;
}

void ScreenRenderComponentObj::on_release(void* user)
{
    ScreenRenderComponentObj* obj = (ScreenRenderComponentObj*)user;

    if (!obj->hasInit)
        return;

    obj->hasInit = false;
    RDevice device = obj->device;

    for (Frame& frame : obj->frames)
    {
        for (RBuffer vbo : frame.rectVBOs)
        {
            vbo.unmap();
            device.destroy_buffer(vbo);
        }
        frame.rectVBOs.clear();
    }

    device.destroy_image(obj->whitePixel);
    device.destroy_set_pool(obj->setPool);
    device.destroy_pipeline(obj->rectPipeline);
    device.destroy_shader(obj->rectVS);
    device.destroy_shader(obj->rectFS);

    device.destroy_buffer(obj->rectIBO);
}

void ScreenRenderComponentObj::on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData)
{
    ScreenRenderComponentObj* obj = (ScreenRenderComponentObj*)userData;
    Frame& frame = obj->frames[obj->frameIdx];

    list.cmd_bind_graphics_pipeline(obj->rectPipeline);
    list.cmd_bind_index_buffer(obj->rectIBO, RINDEX_TYPE_U32);

    obj->rectBatch.reset();
    obj->batchIdx = 0;
    obj->imageCounter = 0;
    obj->list = list;
    obj->graphicsPass = pass;
    obj->on_draw({obj}, obj->user);
    obj->graphicsPass = {};
    obj->flush_rects();
}

ScreenRenderComponent ScreenRenderComponent::add(RGraph graph, RFormat format, OnDrawCallback onDraw, void* user, bool hasSampledImage)
{
    LD_PROFILE_SCOPE;

    RDevice device = graph.get_device();
    uint32_t screenWidth, screenHeight;
    graph.get_screen_extent(screenWidth, screenHeight);

    sSRCompObj.init(device);
    sSRCompObj.frameIdx = device.get_frame_index();
    sSRCompObj.user = user;
    sSRCompObj.imageCounter = 0;
    sSRCompObj.hasSampledImage = hasSampledImage;

    ScreenRenderComponent render2DComp(&sSRCompObj);

    RComponent comp = graph.add_component(render2DComp.component_name());
    comp.add_io_image(render2DComp.io_name(), format, screenWidth, screenHeight);

    RGraphicsPassInfo gpI{};
    gpI.name = render2DComp.component_name();
    gpI.width = screenWidth;
    gpI.height = screenHeight;

    // draw in screen space on top of previous content
    RGraphicsPass pass = comp.add_graphics_pass(gpI, &sSRCompObj, &ScreenRenderComponentObj::on_graphics_pass);
    pass.use_color_attachment(render2DComp.io_name(), RATTACHMENT_LOAD_OP_LOAD, nullptr);

    // conditional input image with the same dimensions as color attachment
    if (hasSampledImage)
    {
        comp.add_input_image(render2DComp.sampled_name(), format, screenWidth, screenHeight);
        pass.use_image_sampled(render2DComp.sampled_name());
    }

    sSRCompObj.on_draw = onDraw;

    return render2DComp;
}

RImage ScreenRenderComponent::get_sampled_image()
{
    LD_ASSERT(sSRCompObj.hasSampledImage && sSRCompObj.graphicsPass);

    return sSRCompObj.graphicsPass.get_image(ScreenRenderComponent(&sSRCompObj).sampled_name());
}

void ScreenRenderComponent::draw_rect(const Rect& rect, Color color)
{
    if (mObj->rectBatch.is_full())
        mObj->flush_rects();

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    RectVertex* v = mObj->rectBatch.write_rect();
    v[0] = {x0, y0, 0, 0, color, 0}; // TL
    v[1] = {x1, y0, 0, 0, color, 0}; // TR
    v[2] = {x1, y1, 0, 0, color, 0}; // BR
    v[3] = {x0, y1, 0, 0, color, 0}; // BL
}

void ScreenRenderComponent::draw_rect_outline(const Rect& rect, float border, Color color)
{
    if (mObj->rectBatch.get_rect_count() + 4 > mObj->rectBatch.get_max_rect_count())
        mObj->flush_rects();

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    RectVertex* barT = mObj->rectBatch.write_rect();
    barT[0] = {x0, y0, 0, 0, color, 0};
    barT[1] = {x1, y0, 0, 0, color, 0};
    barT[2] = {x1, y0 + border, 0, 0, color, 0};
    barT[3] = {x0, y0 + border, 0, 0, color, 0};

    RectVertex* barB = mObj->rectBatch.write_rect();
    barB[0] = {x0, y1 - border, 0, 0, color, 0};
    barB[1] = {x1, y1 - border, 0, 0, color, 0};
    barB[2] = {x1, y1, 0, 0, color, 0};
    barB[3] = {x0, y1, 0, 0, color, 0};

    RectVertex* barL = mObj->rectBatch.write_rect();
    barL[0] = {x0, y0 + border, 0, 0, color, 0};
    barL[1] = {x0 + border, y0 + border, 0, 0, color, 0};
    barL[2] = {x0 + border, y1 - border, 0, 0, color, 0};
    barL[3] = {x0, y1 - border, 0, 0, color, 0};

    RectVertex* barR = mObj->rectBatch.write_rect();
    barR[0] = {x1 - border, y0 + border, 0, 0, color, 0};
    barR[1] = {x1, y0 + border, 0, 0, color, 0};
    barR[2] = {x1, y1 - border, 0, 0, color, 0};
    barR[3] = {x1 - border, y1 - border, 0, 0, color, 0};
}

void ScreenRenderComponent::draw_image(const Rect& rect, RImage image)
{
    if (mObj->rectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    uint32_t control = get_rect_vertex_control_bits(imageIdx, RECT_VERTEX_IMAGE_HINT_NONE, 0);
    uint32_t white = 0xFFFFFFFF;

    RectVertex* v = mObj->rectBatch.write_rect();
    v[0] = {x0, y0, 0.0f, 0.0f, white, control}; // TL
    v[1] = {x1, y0, 1.0f, 0.0f, white, control}; // TR
    v[2] = {x1, y1, 1.0f, 1.0f, white, control}; // BR
    v[3] = {x0, y1, 0.0f, 1.0f, white, control}; // BL
}

void ScreenRenderComponent::draw_image_uv(const Rect& rect, RImage image, const Rect& uv, Color color)
{
    if (mObj->rectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;
    float u0 = uv.x;
    float u1 = uv.x + uv.w;
    float v0 = uv.y;
    float v1 = uv.y + uv.h;

    uint32_t control = get_rect_vertex_control_bits(imageIdx, RECT_VERTEX_IMAGE_HINT_NONE, 0);

    RectVertex* v = mObj->rectBatch.write_rect();
    v[0] = {x0, y0, u0, v0, color, control}; // TL
    v[1] = {x1, y0, u1, v0, color, control}; // TR
    v[2] = {x1, y1, u1, v1, color, control}; // BR
    v[3] = {x0, y1, u0, v1, color, control}; // BL
}

void ScreenRenderComponent::draw_glyph(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& pos, uint32_t code, Color color)
{
    if (mObj->rectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(atlasImage);
    LD_ASSERT(imageIdx >= 0);

    float filterRatio = atlas.get_filter_ratio(fontSize);
    float aw = (float)atlasImage.width();
    float ah = (float)atlasImage.height();
    IRect glyphBB;
    atlas.get_atlas_glyph(code, glyphBB);
    float u0 = (glyphBB.x / aw);
    float u1 = (glyphBB.x + glyphBB.w) / aw;
    float v0 = (glyphBB.y / ah);
    float v1 = (glyphBB.y + glyphBB.h) / ah;
    float x0 = pos.x;
    float y0 = pos.y;
    float x1 = pos.x + glyphBB.w * filterRatio;
    float y1 = pos.y + glyphBB.h * filterRatio;

    RectVertexImageHint hint = RECT_VERTEX_IMAGE_HINT_NONE;

    switch (atlas.type())
    {
    case FONT_ATLAS_BITMAP:
        hint = RECT_VERTEX_IMAGE_HINT_FONT;
        break;
    case FONT_ATLAS_SDF:
        hint = RECT_VERTEX_IMAGE_HINT_FONT_SDF;
        break;
    }

    uint32_t control = get_rect_vertex_control_bits(imageIdx, hint, filterRatio);

    RectVertex* v = mObj->rectBatch.write_rect();
    v[0] = {x0, y0, u0, v0, color, control}; // TL
    v[1] = {x1, y0, u1, v0, color, control}; // TR
    v[2] = {x1, y1, u1, v1, color, control}; // BR
    v[3] = {x0, y1, u0, v1, color, control}; // BL
}

float ScreenRenderComponent::draw_glyph_baseline(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& baseline, uint32_t code, Color color)
{
    float advanceX;
    Rect rect;
    atlas.get_baseline_glyph(code, fontSize, baseline, rect, advanceX);
    draw_glyph(atlas, atlasImage, fontSize, rect.get_pos(), code, color);

    return advanceX;
}

void ScreenRenderComponent::draw_text(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& pos, const char* text, Color color, float wrapWidth)
{
    Font f = atlas.get_font();
    FontMetrics metrics;
    f.get_metrics(metrics, fontSize);

    Vec2 baseline(pos.x, pos.y + metrics.ascent);

    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++)
    {
        uint32_t c = (uint32_t)text[i];

        float advanceX;
        Rect rect;
        atlas.get_baseline_glyph(c, fontSize, baseline, rect, advanceX);

        bool shouldWrap = wrapWidth > 0.0f && (baseline.x + advanceX - pos.x) > wrapWidth;

        if (c == '\n' || shouldWrap)
        {
            baseline.y += metrics.lineHeight;
            baseline.x = pos.x;
            atlas.get_baseline_glyph(c, fontSize, baseline, rect, advanceX);
            continue;
        }

        draw_glyph(atlas, atlasImage, fontSize, rect.get_pos(), c, color);

        baseline.x += advanceX;
    }
}

} // namespace LD
