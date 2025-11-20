#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/System/Memory.h>
#include <array>
#include <stack>
#include <vector>

#define IMAGE_SLOT_COUNT 8

namespace LD {

// clang-format off
static const char sRectVSSource[] = R"(
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

static const char sRectFSSource[] = R"(
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

static RSetBindingInfo sScreenSetBinding = {0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, IMAGE_SLOT_COUNT};
static RSetLayoutInfo sScreenSetLayout = {.bindingCount = 1, .bindings = &sScreenSetBinding};
static RDevice sDevice;
static RShader sRectVS;
static RShader sRectFS;
static RPipeline sRectPipeline;
static RImage sWhitePixel;
static bool sHasStaticStartup;
static std::unordered_map<Hash32, ScreenRenderComponentObj*> sInstances;
static RPipelineLayoutInfo sScreenPipelineLayout;

/// @brief Screen render component instance.
class ScreenRenderComponentObj
{
    friend class ScreenRenderComponent;

public:
    ScreenRenderComponentObj(RDevice device, const char* name);
    ~ScreenRenderComponentObj();

    static void static_startup(RDevice device);
    static void static_cleanup(void* user);
    static void on_destroy(void* user);
    static void on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData);

private: // instance members
    struct Batch
    {
        RBuffer rectVBO;
        RSet screenSet;
        RSetPool screenSetPool;
    };

    struct Frame
    {
        std::vector<Batch> batches;
    };

    RBuffer mRectIBO;
    RCommandList mList;
    RImage mImageSlots[IMAGE_SLOT_COUNT];
    RectVertexBatch<sMaxRectCount> mRectBatch;
    RGraphicsPass mGraphicsPass;
    uint32_t mImageCounter;
    uint32_t mBatchIdx;
    uint32_t mFrameIdx;
    uint32_t mScreenWidth;
    uint32_t mScreenHeight;
    Color mColorMask = 0xFFFFFFFF;
    std::string mName;
    std::vector<Frame> mFrames;
    std::stack<Rect> mScissors;
    std::stack<Color> mColorMasks;
    void (*mOnDraw)(ScreenRenderComponent renderer, void* user);
    void* mUser;
    bool mHasSampledImage;
    bool mHasInputImage;

    void flush_rects();

    int get_image_index(RImage image);
};

ScreenRenderComponentObj::ScreenRenderComponentObj(RDevice device, const char* name)
{
    ScreenRenderComponentObj::static_startup(device);

    uint32_t* indices = (uint32_t*)heap_malloc(sizeof(uint32_t) * sMaxRectIndexCount, MEMORY_USAGE_RENDER);
    mRectBatch.write_indices(indices);
    mBatchIdx = 0;
    mImageCounter = 0;
    mList = {};
    mName = "screen_render_";
    mName += name;

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT,
        .size = sizeof(uint32_t) * sMaxRectIndexCount,
        .hostVisible = false,
    };
    mRectIBO = device.create_buffer(bufferI);

    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    stager.add_buffer_data(mRectIBO, indices);
    heap_free(indices);

    stager.submit(device.get_graphics_queue());

    mFrames.resize(device.get_frames_in_flight_count());
    for (Frame& frame : mFrames)
    {
        bufferI = {
            .usage = RBUFFER_USAGE_VERTEX_BIT,
            .size = sizeof(RectVertex) * sMaxRectVertexCount,
            .hostVisible = true, // persistent mapping
        };
        frame.batches.resize(1);
        Batch& firstBatch = frame.batches.front();
        firstBatch.rectVBO = device.create_buffer(bufferI);
        firstBatch.rectVBO.map();
        firstBatch.screenSetPool = device.create_set_pool({
            .layout = sScreenSetLayout,
            .maxSets = 1,
        });
        firstBatch.screenSet = firstBatch.screenSetPool.allocate();
    }
}

ScreenRenderComponentObj::~ScreenRenderComponentObj()
{
    sDevice.destroy_buffer(mRectIBO);

    for (Frame& frame : mFrames)
    {
        for (Batch batch : frame.batches)
        {
            batch.rectVBO.unmap();
            sDevice.destroy_buffer(batch.rectVBO);
            sDevice.destroy_set_pool(batch.screenSetPool);
        }
        frame.batches.clear();
    }
}

void ScreenRenderComponentObj::flush_rects()
{
    LD_PROFILE_SCOPE;

    Frame& frame = mFrames[mFrameIdx];
    Batch& batch = frame.batches[mBatchIdx];

    uint32_t rectCount = mRectBatch.get_rect_count();
    uint32_t vertexCount;
    RectVertex* vertices = mRectBatch.get_vertices(vertexCount);

    if (vertexCount == 0)
        return;

    batch.rectVBO.map_write(0, sizeof(RectVertex) * vertexCount, vertices);

    mRectBatch.reset();

    RImageLayout layouts[IMAGE_SLOT_COUNT];
    std::fill(layouts, layouts + IMAGE_SLOT_COUNT, RIMAGE_LAYOUT_SHADER_READ_ONLY);

    RSetImageUpdateInfo updateI{};
    updateI.set = batch.screenSet;
    updateI.dstBinding = 0;
    updateI.dstArrayIndex = 0;
    updateI.imageCount = IMAGE_SLOT_COUNT;
    updateI.imageLayouts = layouts;
    updateI.imageBindingType = RBINDING_TYPE_COMBINED_IMAGE_SAMPLER;
    updateI.images = mImageSlots;
    sDevice.update_set_images(1, &updateI);

    mList.cmd_bind_vertex_buffers(0, 1, &batch.rectVBO);
    mList.cmd_bind_graphics_sets(sScreenPipelineLayout, 1, 1, &batch.screenSet);

    RDrawIndexedInfo drawI = {
        .indexCount = rectCount * 6,
        .indexStart = 0,
        .instanceCount = 1,
        .instanceStart = 0,
    };
    mList.cmd_draw_indexed(drawI);

    if (++mBatchIdx < frame.batches.size())
        return;

    frame.batches.push_back({});
    Batch& newBatch = frame.batches.back();

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_VERTEX_BIT,
        .size = sizeof(RectVertex) * sMaxRectVertexCount,
        .hostVisible = true, // persistent mapping
    };

    newBatch.rectVBO = sDevice.create_buffer(bufferI);
    newBatch.rectVBO.map();
    newBatch.screenSetPool = sDevice.create_set_pool({
        .layout = sScreenSetLayout,
        .maxSets = 1,
    });
    newBatch.screenSet = newBatch.screenSetPool.allocate();
}

int ScreenRenderComponentObj::get_image_index(RImage image)
{
    for (int i = 0; i < mImageCounter; i++)
    {
        if (mImageSlots[i] == image)
            return i + 1;
    }

    if (mImageCounter == 8)
        return -1; // caller should flush

    mImageSlots[mImageCounter++] = image;

    return mImageCounter;
}

void ScreenRenderComponentObj::static_startup(RDevice device)
{
    if (sHasStaticStartup)
        return;

    sHasStaticStartup = true;
    sDevice = device;

    RGraph::add_release_callback(nullptr, &ScreenRenderComponentObj::static_cleanup);

    sRectVS = device.create_shader({.type = RSHADER_TYPE_VERTEX, .glsl = sRectVSSource});
    sRectFS = device.create_shader({.type = RSHADER_TYPE_FRAGMENT, .glsl = sRectFSSource});

    std::array<RShader, 2> shaders{sRectVS, sRectFS};
    std::vector<RVertexAttribute> attrs;
    RVertexBinding binding = {.inputRate = RBINDING_INPUT_RATE_VERTEX, .stride = sizeof(RectVertex)};
    get_rect_vertex_attributes(attrs);

    static RSetLayoutInfo setLayouts[2];
    setLayouts[0] = sFrameSetLayout;
    setLayouts[1] = sScreenSetLayout;

    sScreenPipelineLayout.setLayoutCount = 2;
    sScreenPipelineLayout.setLayouts = setLayouts;

    RPipelineBlendState blendState = RUtil::make_default_blend_state();

    RPipelineInfo pipelineI{
        .shaderCount = (uint32_t)shaders.size(),
        .shaders = shaders.data(),
        .vertexAttributeCount = (uint32_t)attrs.size(),
        .vertexAttributes = attrs.data(),
        .vertexBindingCount = 1,
        .vertexBindings = &binding,
        .layout = sScreenPipelineLayout,
        .depthStencil = {
            .depthTestEnabled = false,
        },
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendState,
        },
    };

    sRectPipeline = device.create_pipeline(pipelineI);

    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, 1, 1, {});
    sWhitePixel = device.create_image(imageI);
    uint32_t pixel = 0xFFFFFFFF;
    stager.add_image_data(sWhitePixel, &pixel, RIMAGE_LAYOUT_SHADER_READ_ONLY);
    stager.submit(device.get_graphics_queue());
}

void ScreenRenderComponentObj::static_cleanup(void* user)
{
    if (!sHasStaticStartup)
        return;

    sHasStaticStartup = false;

    for (auto ite : sInstances)
    {
        ScreenRenderComponentObj* obj = ite.second;
        heap_delete<ScreenRenderComponentObj>(obj);
    }

    sInstances.clear();

    sDevice.destroy_image(sWhitePixel);
    sDevice.destroy_pipeline(sRectPipeline);
    sDevice.destroy_shader(sRectVS);
    sDevice.destroy_shader(sRectFS);
    sDevice = {};
}

void ScreenRenderComponentObj::on_graphics_pass(RGraphicsPass pass, RCommandList list, void* user)
{
    auto* obj = (ScreenRenderComponentObj*)user;
    Frame& frame = obj->mFrames[obj->mFrameIdx];

    list.cmd_bind_graphics_pipeline(sRectPipeline);
    list.cmd_bind_index_buffer(obj->mRectIBO, RINDEX_TYPE_U32);

    std::fill(obj->mImageSlots, obj->mImageSlots + IMAGE_SLOT_COUNT, sWhitePixel);

    obj->mRectBatch.reset();
    obj->mBatchIdx = 0;
    obj->mImageCounter = 0;
    obj->mList = list;
    obj->mGraphicsPass = pass;
    obj->mOnDraw({obj}, obj->mUser);
    obj->mGraphicsPass = {};
    obj->flush_rects();
}

ScreenRenderComponent ScreenRenderComponent::add(RGraph graph, const ScreenRenderComponentInfo& info)
{
    LD_PROFILE_SCOPE;

    ScreenRenderComponentObj* obj = nullptr;
    Hash32 nameHash(info.name);

    auto ite = sInstances.find(nameHash);
    if (ite != sInstances.end())
    {
        obj = ite->second;
    }
    else
    {
        obj = heap_new<ScreenRenderComponentObj>(MEMORY_USAGE_RENDER, graph.get_device(), info.name);
        sInstances[nameHash] = obj;
    }

    RDevice device = graph.get_device();
    graph.get_screen_extent(obj->mScreenWidth, obj->mScreenHeight);

    obj->mFrameIdx = device.get_frame_index();
    obj->mUser = info.user;
    obj->mOnDraw = info.onDrawCallback;
    obj->mImageCounter = 0;
    obj->mHasInputImage = info.hasInputImage;
    obj->mHasSampledImage = info.hasSampledImage;

    ScreenRenderComponent screenRC(obj);

    RComponent comp = graph.add_component(screenRC.component_name());

    if (obj->mHasInputImage)
        comp.add_io_image(screenRC.io_name(), info.format, obj->mScreenWidth, obj->mScreenHeight);
    else
        comp.add_output_image(screenRC.io_name(), info.format, obj->mScreenWidth, obj->mScreenHeight);

    RGraphicsPassInfo gpI{};
    gpI.name = screenRC.component_name();
    gpI.width = obj->mScreenWidth;
    gpI.height = obj->mScreenHeight;

    RGraphicsPass pass = comp.add_graphics_pass(gpI, obj, &ScreenRenderComponentObj::on_graphics_pass);
    if (obj->mHasInputImage)
    {
        // draw in screen space on top of previous image content
        pass.use_color_attachment(screenRC.io_name(), RATTACHMENT_LOAD_OP_LOAD, nullptr);
    }
    else
    {
        // use clear color to initialize new image content
        RClearColorValue tmpClearColor = RUtil::make_clear_color(info.clearColor);
        pass.use_color_attachment(screenRC.io_name(), RATTACHMENT_LOAD_OP_CLEAR, &tmpClearColor);
    }

    if (obj->mHasSampledImage)
    {
        // conditional input image with the same dimensions as color attachment
        comp.add_input_image(screenRC.sampled_name(), info.format, obj->mScreenWidth, obj->mScreenHeight);
        pass.use_image_sampled(screenRC.sampled_name());
    }

    return screenRC;
}

const char* ScreenRenderComponent::component_name() const
{
    return mObj->mName.c_str();
}

RImage ScreenRenderComponent::get_sampled_image()
{
    LD_ASSERT(mObj->mHasSampledImage && mObj->mGraphicsPass);

    return mObj->mGraphicsPass.get_image(this->sampled_name());
}

void ScreenRenderComponent::get_screen_extent(uint32_t& screenWidth, uint32_t& screenHeight)
{
    screenWidth = mObj->mScreenWidth;
    screenHeight = mObj->mScreenHeight;
}

void ScreenRenderComponent::push_scissor(const Rect& scissor)
{
    LD_ASSERT(mObj->mList);

    // flush current batch before changing scissor state
    mObj->flush_rects();

    mObj->mScissors.push(scissor);
    mObj->mList.cmd_set_scissor(scissor);
}

void ScreenRenderComponent::pop_scissor()
{
    LD_ASSERT(mObj->mList);

    if (mObj->mScissors.empty())
        return;

    // flush current batch before changing scissor state
    mObj->flush_rects();

    mObj->mScissors.pop();

    if (mObj->mScissors.empty())
    {
        Rect scissor(0.0f, 0.0f, (float)mObj->mScreenWidth, (float)mObj->mScreenHeight);
        mObj->mList.cmd_set_scissor(scissor);
    }
    else
    {
        Rect topScissor = mObj->mScissors.top();
        mObj->mList.cmd_set_scissor(topScissor);
    }
}

void ScreenRenderComponent::push_color_mask(Color mask)
{
    LD_ASSERT(mObj->mList);

    mObj->mColorMasks.push(mask);
    mObj->mColorMask = mask;
}

void ScreenRenderComponent::pop_color_mask()
{
    LD_ASSERT(mObj->mList);

    if (mObj->mColorMasks.empty())
        return;

    mObj->mColorMasks.pop();

    if (mObj->mColorMasks.empty())
        mObj->mColorMask = 0xFFFFFFFF;
    else
        mObj->mColorMask = mObj->mColorMasks.top();
}

void ScreenRenderComponent::draw_rect(const Rect& rect, Color color)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    color = color * mObj->mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    RectVertex* v = mObj->mRectBatch.write_rect();
    v[0] = {x0, y0, 0, 0, color, 0}; // TL
    v[1] = {x1, y0, 0, 0, color, 0}; // TR
    v[2] = {x1, y1, 0, 0, color, 0}; // BR
    v[3] = {x0, y1, 0, 0, color, 0}; // BL
}

void ScreenRenderComponent::draw_rect_outline(const Rect& rect, float border, Color color)
{
    if (mObj->mRectBatch.get_rect_count() + 4 > mObj->mRectBatch.get_max_rect_count())
        mObj->flush_rects();

    color = color * mObj->mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    RectVertex* barT = mObj->mRectBatch.write_rect();
    barT[0] = {x0, y0, 0, 0, color, 0};
    barT[1] = {x1, y0, 0, 0, color, 0};
    barT[2] = {x1, y0 + border, 0, 0, color, 0};
    barT[3] = {x0, y0 + border, 0, 0, color, 0};

    RectVertex* barB = mObj->mRectBatch.write_rect();
    barB[0] = {x0, y1 - border, 0, 0, color, 0};
    barB[1] = {x1, y1 - border, 0, 0, color, 0};
    barB[2] = {x1, y1, 0, 0, color, 0};
    barB[3] = {x0, y1, 0, 0, color, 0};

    RectVertex* barL = mObj->mRectBatch.write_rect();
    barL[0] = {x0, y0 + border, 0, 0, color, 0};
    barL[1] = {x0 + border, y0 + border, 0, 0, color, 0};
    barL[2] = {x0 + border, y1 - border, 0, 0, color, 0};
    barL[3] = {x0, y1 - border, 0, 0, color, 0};

    RectVertex* barR = mObj->mRectBatch.write_rect();
    barR[0] = {x1 - border, y0 + border, 0, 0, color, 0};
    barR[1] = {x1, y0 + border, 0, 0, color, 0};
    barR[2] = {x1, y1 - border, 0, 0, color, 0};
    barR[3] = {x1 - border, y1 - border, 0, 0, color, 0};
}

void ScreenRenderComponent::draw_image(const Rect& rect, RImage image, Color color)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    uint32_t control = get_rect_vertex_control_bits(imageIdx, RECT_VERTEX_IMAGE_HINT_NONE, 0);
    uint32_t tint = color * mObj->mColorMask;

    RectVertex* v = mObj->mRectBatch.write_rect();
    v[0] = {x0, y0, 0.0f, 0.0f, tint, control};  // TL
    v[1] = {x1, y0, 1.0f, 0.0f, tint, control};  // TR
    v[2] = {x1, y1, 1.0f, 1.0f, tint, control};  // BR
    v[3] = {x0, y1, 0.0f, 1.0f, tint, control};  // BL
}

void ScreenRenderComponent::draw_image_uv(const Rect& rect, RImage image, const Rect& uv, Color color)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    color = color * mObj->mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;
    float u0 = uv.x;
    float u1 = uv.x + uv.w;
    float v0 = uv.y;
    float v1 = uv.y + uv.h;

    uint32_t control = get_rect_vertex_control_bits(imageIdx, RECT_VERTEX_IMAGE_HINT_NONE, 0);

    RectVertex* v = mObj->mRectBatch.write_rect();
    v[0] = {x0, y0, u0, v0, color, control}; // TL
    v[1] = {x1, y0, u1, v0, color, control}; // TR
    v[2] = {x1, y1, u1, v1, color, control}; // BR
    v[3] = {x0, y1, u0, v1, color, control}; // BL
}

// NOTE: this function applies the color mask,
//       if caller also applies the color mask
//       we will have the mask incorrectly applied twice.
void ScreenRenderComponent::draw_glyph(FontAtlas atlas, RImage atlasImage, float fontSize, const Vec2& pos, uint32_t code, Color color)
{
    if (mObj->mRectBatch.is_full())
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

    color = color * mObj->mColorMask;

    uint32_t control = get_rect_vertex_control_bits(imageIdx, hint, filterRatio);

    RectVertex* v = mObj->mRectBatch.write_rect();
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
    if (!text)
        return;

    Font f = atlas.get_font();
    FontMetrics metrics;
    f.get_metrics(metrics, fontSize);

    Vec2 baseline(pos.x, pos.y + metrics.ascent);

    // add small bias to ensure that floating point errors
    // do not cause the last character in single-line text to wrap.
    wrapWidth += 0.1f;

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
