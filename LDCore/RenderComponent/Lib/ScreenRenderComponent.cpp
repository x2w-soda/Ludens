#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/Pipeline/QuadPipeline.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>

namespace LD {

static constexpr uint32_t sMaxQuadCount = 2048;
static constexpr uint32_t sMaxQuadVertexCount = sMaxQuadCount * 4;
static constexpr uint32_t sMaxQuadIndexCount = sMaxQuadCount * 6;
static constexpr size_t sImageSlotCount = QuadPipeline::image_slots();

static RDevice sDevice;
static RImage sWhitePixel;
static QuadPipeline sQuadPipelineUber;
static QuadPipeline sQuadPipelineRect;
static bool sHasStaticStartup;
static HashMap<Hash32, ScreenRenderComponentObj*> sInstances;

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
        RSetPool imageSlotSetPool;
    };

    struct Frame
    {
        Vector<Batch> batches;
    };

    RBuffer mRectIBO;
    RCommandList mList;
    RImage mImageSlots[sImageSlotCount];
    QuadVertexBatch<sMaxQuadCount> mRectBatch;
    QuadPipelineType mPipelineType = QUAD_PIPELINE_TYPE_ENUM_COUNT;
    RGraphicsPass mGraphicsPass;
    RGraphImage mColorAttachment{};
    RGraphImage mSampledAttachment{};
    uint32_t mImageCounter;
    uint32_t mBatchIdx;
    uint32_t mFrameIdx;
    uint32_t mScreenWidth;
    uint32_t mScreenHeight;
    Color mColorMask = 0xFFFFFFFF;
    std::string mName;
    Vector<Frame> mFrames;
    Stack<Rect> mScissors;
    Stack<Rect> mViewports;
    Stack<Color> mColorMasks;
    void (*mOnDraw)(ScreenRenderComponent renderer, void* user);
    void* mUser;
    bool mHasSampledImage;
    bool mHasInputImage;

    void flush_rects();
    void write_quad_image(const Rect& rect, Color color, int imageIndex, const Rect& uv, bool forceAlphaOne);
    void write_quad_rounded(const Rect& rect, Color color, int imageIndex, const Rect& uv, float radius);
    void write_quad_ellipse(const Rect& rect, Color color, int imageIndex, const Rect& uv);

    int get_image_index(RImage image);
    inline const char* io_name() const { return "IO"; }
    inline const char* sampled_name() const { return "Sampled"; }
};

ScreenRenderComponentObj::ScreenRenderComponentObj(RDevice device, const char* name)
{
    ScreenRenderComponentObj::static_startup(device);

    uint32_t* indices = (uint32_t*)heap_malloc(sizeof(uint32_t) * sMaxQuadIndexCount, MEMORY_USAGE_RENDER);
    mRectBatch.write_indices(indices);
    mBatchIdx = 0;
    mImageCounter = 0;
    mList = {};
    mName = "SRC";
    mName += name;

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT,
        .size = sizeof(uint32_t) * sMaxQuadIndexCount,
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
            .size = sizeof(QuadVertex) * sMaxQuadVertexCount,
            .hostVisible = true, // persistent mapping
        };
        frame.batches.resize(1);
        Batch& firstBatch = frame.batches.front();
        firstBatch.rectVBO = device.create_buffer(bufferI);
        firstBatch.rectVBO.map();
        firstBatch.imageSlotSetPool = device.create_set_pool({
            .layout = QuadPipeline::image_slot_set_layout(),
            .maxSets = 1,
        });
        firstBatch.screenSet = firstBatch.imageSlotSetPool.allocate();
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
            sDevice.destroy_set_pool(batch.imageSlotSetPool);
        }
        frame.batches.clear();
    }
}

void ScreenRenderComponentObj::flush_rects()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(mPipelineType != QUAD_PIPELINE_TYPE_ENUM_COUNT); // forgot bind_quad_pipeline()

    Frame& frame = mFrames[mFrameIdx];
    Batch& batch = frame.batches[mBatchIdx];

    uint32_t rectCount = mRectBatch.get_quad_count();
    uint32_t vertexCount;
    QuadVertex* vertices = mRectBatch.get_vertices(vertexCount);

    if (vertexCount == 0)
        return;

    batch.rectVBO.map_write(0, sizeof(QuadVertex) * vertexCount, vertices);

    mRectBatch.reset();

    RImageLayout layouts[sImageSlotCount];
    std::fill(layouts, layouts + sImageSlotCount, RIMAGE_LAYOUT_SHADER_READ_ONLY);

    RSetImageUpdateInfo updateI{};
    updateI.set = batch.screenSet;
    updateI.dstBinding = 0;
    updateI.dstArrayIndex = 0;
    updateI.imageCount = sImageSlotCount;
    updateI.imageLayouts = layouts;
    updateI.imageBindingType = RBINDING_TYPE_COMBINED_IMAGE_SAMPLER;
    updateI.images = mImageSlots;
    sDevice.update_set_images(1, &updateI);

    mList.cmd_bind_vertex_buffers(0, 1, &batch.rectVBO);
    mList.cmd_bind_graphics_sets(QuadPipeline::layout(), 1, 1, &batch.screenSet);

    RDrawIndexedInfo drawI = {
        .indexCount = rectCount * 6,
        .instanceCount = 1,
        .indexStart = 0,
        .vertexOffset = 0,
        .instanceStart = 0,
    };
    mList.cmd_draw_indexed(drawI);

    if (++mBatchIdx < frame.batches.size())
        return;

    frame.batches.push_back({});
    Batch& newBatch = frame.batches.back();

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_VERTEX_BIT,
        .size = sizeof(QuadVertex) * sMaxQuadVertexCount,
        .hostVisible = true, // persistent mapping
    };

    newBatch.rectVBO = sDevice.create_buffer(bufferI);
    newBatch.rectVBO.map();
    newBatch.imageSlotSetPool = sDevice.create_set_pool({
        .layout = QuadPipeline::image_slot_set_layout(),
        .maxSets = 1,
    });
    newBatch.screenSet = newBatch.imageSlotSetPool.allocate();
}

void ScreenRenderComponentObj::write_quad_image(const Rect& rect, Color color, int imageIndex, const Rect& uv, bool forceAlphaOne)
{
    LD_ASSERT(!mRectBatch.is_full());

    color = color * mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;
    float u0 = uv.x;
    float u1 = uv.x + uv.w;
    float v0 = uv.y;
    float v1 = uv.y + uv.h;

    QuadMode mode = forceAlphaOne ? QUAD_MODE_FORCE_ALPHA_ONE : QUAD_MODE_NONE;
    UVec4 control = get_quad_vertex_control_bits(imageIndex, mode, 0.0f, 0.0f);

    QuadVertex* v = mRectBatch.write_quad();
    v[0] = {x0, y0, u0, v0, color, control[0]}; // TL
    v[1] = {x1, y0, u1, v0, color, control[1]}; // TR
    v[2] = {x1, y1, u1, v1, color, control[2]}; // BR
    v[3] = {x0, y1, u0, v1, color, control[3]}; // BL
}

void ScreenRenderComponentObj::write_quad_rounded(const Rect& rect, Color color, int imageIndex, const Rect& uv, float radius)
{
    LD_ASSERT(!mRectBatch.is_full());

    color = color * mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;
    float u0 = uv.x;
    float u1 = uv.x + uv.w;
    float v0 = uv.y;
    float v1 = uv.y + uv.h;

    // normalize a 0.1-10.0 aspect ratio
    float aspect = 10.0f;
    if (rect.h != 0)
        aspect = std::clamp(rect.w / rect.h, 0.1f, 10.0f);
    aspect = (aspect - 0.1f) / 9.9f;

    UVec4 control = get_quad_vertex_control_bits(imageIndex, QUAD_MODE_RECT_ROUNDED, aspect, radius);

    QuadVertex* v = mRectBatch.write_quad();
    v[0] = {x0, y0, u0, v0, color, control[0]}; // TL
    v[1] = {x1, y0, u1, v0, color, control[1]}; // TR
    v[2] = {x1, y1, u1, v1, color, control[2]}; // BR
    v[3] = {x0, y1, u0, v1, color, control[3]}; // BL
}

void ScreenRenderComponentObj::write_quad_ellipse(const Rect& rect, Color color, int imageIndex, const Rect& uv)
{
    LD_ASSERT(!mRectBatch.is_full());

    color = color * mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;
    float u0 = uv.x;
    float u1 = uv.x + uv.w;
    float v0 = uv.y;
    float v1 = uv.y + uv.h;

    UVec4 control = get_quad_vertex_control_bits(imageIndex, QUAD_MODE_ELLIPSE, 0.0f, 0.0f);

    QuadVertex* v = mRectBatch.write_quad();
    v[0] = {x0, y0, u0, v0, color, control[0]}; // TL
    v[1] = {x1, y0, u1, v0, color, control[1]}; // TR
    v[2] = {x1, y1, u1, v1, color, control[2]}; // BR
    v[3] = {x0, y1, u0, v1, color, control[3]}; // BL
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

    sQuadPipelineUber = QuadPipeline::create(device, QUAD_PIPELINE_UBER);
    sQuadPipelineRect = QuadPipeline::create(device, QUAD_PIPELINE_RECT);

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
    QuadPipeline::destroy(sQuadPipelineRect);
    sQuadPipelineRect = {};
    QuadPipeline::destroy(sQuadPipelineUber);
    sQuadPipelineUber = {};
    sDevice = {};
}

void ScreenRenderComponentObj::on_graphics_pass(RGraphicsPass pass, RCommandList list, void* user)
{
    auto* obj = (ScreenRenderComponentObj*)user;
    Frame& frame = obj->mFrames[obj->mFrameIdx];

    obj->mPipelineType = QUAD_PIPELINE_TYPE_ENUM_COUNT;

    list.cmd_bind_index_buffer(obj->mRectIBO, RINDEX_TYPE_U32);

    std::fill(obj->mImageSlots, obj->mImageSlots + sImageSlotCount, sWhitePixel);

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

    if (info.screenExtent)
    {
        obj->mScreenWidth = (uint32_t)info.screenExtent->x;
        obj->mScreenHeight = (uint32_t)info.screenExtent->y;
    }
    else
    {
        graph.get_screen_extent(obj->mScreenWidth, obj->mScreenHeight);
    }

    RDevice device = graph.get_device();
    obj->mFrameIdx = device.get_frame_index();
    obj->mUser = info.user;
    obj->mOnDraw = info.onDrawCallback;
    obj->mImageCounter = 0;
    obj->mHasInputImage = info.hasInputImage;
    obj->mHasSampledImage = info.hasSampledImage;

    ScreenRenderComponent screenRC(obj);

    RComponent comp = graph.add_component(screenRC.component_name());

    if (obj->mHasInputImage)
        obj->mColorAttachment = comp.add_io_image(obj->io_name(), info.format, obj->mScreenWidth, obj->mScreenHeight);
    else
        obj->mColorAttachment = comp.add_output_image(obj->io_name(), info.format, obj->mScreenWidth, obj->mScreenHeight);

    RGraphicsPassInfo gpI{};
    gpI.name = "GP";
    gpI.width = obj->mScreenWidth;
    gpI.height = obj->mScreenHeight;

    RGraphicsPass pass = comp.add_graphics_pass(gpI, obj, &ScreenRenderComponentObj::on_graphics_pass);
    if (obj->mHasInputImage)
    {
        // draw in screen space on top of previous image content
        pass.use_color_attachment(obj->mColorAttachment, RATTACHMENT_LOAD_OP_LOAD, nullptr);
    }
    else
    {
        // use clear color to initialize new image content
        RClearColorValue tmpClearColor = RUtil::make_clear_color(info.clearColor);
        pass.use_color_attachment(obj->mColorAttachment, RATTACHMENT_LOAD_OP_CLEAR, &tmpClearColor);
    }

    if (obj->mHasSampledImage)
    {
        // conditional input image with the same dimensions as color attachment
        obj->mSampledAttachment = comp.add_input_image(obj->sampled_name(), info.format, obj->mScreenWidth, obj->mScreenHeight);
        pass.use_image_sampled(obj->mSampledAttachment);
    }

    return screenRC;
}

const char* ScreenRenderComponent::component_name() const
{
    return mObj->mName.c_str();
}

RGraphImage ScreenRenderComponent::color_attachment()
{
    return mObj->mColorAttachment;
}

RGraphImage ScreenRenderComponent::sampled_attachment()
{
    return mObj->mSampledAttachment;
}

RImage ScreenRenderComponent::get_sampled_image()
{
    LD_ASSERT(mObj->mHasSampledImage && mObj->mGraphicsPass);

    return mObj->mGraphicsPass.get_image(mObj->sampled_name());
}

void ScreenRenderComponent::get_screen_extent(uint32_t& screenWidth, uint32_t& screenHeight)
{
    screenWidth = mObj->mScreenWidth;
    screenHeight = mObj->mScreenHeight;
}

void ScreenRenderComponent::set_view_projection_index(int vpIndex)
{
    LD_ASSERT(mObj->mList);
    LD_ASSERT(vpIndex >= 0); // caller's responsibility

    // flush crrent batch before changing view projection state
    mObj->flush_rects();

    QuadPipeline::PushConstant pc{};
    pc.vpIndex = (uint32_t)vpIndex;
    mObj->mList.cmd_push_constant(QuadPipeline::layout(), 0, sizeof(pc), &pc);
}

void ScreenRenderComponent::push_viewport(const Rect& viewport)
{
    LD_ASSERT(mObj->mList);

    // flush current batch before changing viewport state
    mObj->flush_rects();

    mObj->mViewports.push(viewport);
    mObj->mList.cmd_set_viewport(viewport);
}

void ScreenRenderComponent::push_viewport_normalized(const Rect& viewport)
{
    LD_ASSERT(mObj->mList);

    Rect screenViewport;
    screenViewport.x = viewport.x * (float)mObj->mScreenWidth;
    screenViewport.y = viewport.y * (float)mObj->mScreenHeight;
    screenViewport.w = viewport.w * (float)mObj->mScreenWidth;
    screenViewport.h = viewport.h * (float)mObj->mScreenHeight;

    push_viewport(screenViewport);
}

void ScreenRenderComponent::pop_viewport()
{
    LD_ASSERT(mObj->mList);

    if (mObj->mViewports.empty())
        return;

    // flush current batch before changing viewport state
    mObj->flush_rects();

    mObj->mViewports.pop();

    if (mObj->mViewports.empty())
    {
        Rect viewport(0.0f, 0.0f, (float)mObj->mScreenWidth, (float)mObj->mScreenHeight);
        mObj->mList.cmd_set_viewport(viewport);
    }
    else
    {
        Rect topViewport = mObj->mViewports.top();
        mObj->mList.cmd_set_viewport(topViewport);
    }
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

void ScreenRenderComponent::bind_quad_pipeline(QuadPipelineType type)
{
    LD_ASSERT(mObj->mList);

    if (mObj->mPipelineType == type)
        return;

    mObj->mPipelineType = type;
    mObj->flush_rects();

    switch (type)
    {
    case QUAD_PIPELINE_UBER:
        mObj->mList.cmd_bind_graphics_pipeline(sQuadPipelineUber.handle());
        break;
    case QUAD_PIPELINE_RECT:
        mObj->mList.cmd_bind_graphics_pipeline(sQuadPipelineRect.handle());
        break;
    }
}

QuadVertex* ScreenRenderComponent::draw(RImage image)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    UVec4 control{};

    if (image)
    {
        int imageIdx = mObj->get_image_index(image);
        LD_ASSERT(imageIdx >= 0);
        control = get_quad_vertex_control_bits(imageIdx, QUAD_MODE_NONE, 0.0f, 0.0f);
    }

    // NOTE: this allows user to bypass mObj->mColorMask and write the final color directly
    QuadVertex* v = mObj->mRectBatch.write_quad();
    v[0].control = control[0];
    v[1].control = control[1];
    v[2].control = control[2];
    v[3].control = control[3];

    return v;
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

    QuadVertex* v = mObj->mRectBatch.write_quad();
    v[0] = {x0, y0, 0, 0, color, 0}; // TL
    v[1] = {x1, y0, 0, 0, color, 0}; // TR
    v[2] = {x1, y1, 0, 0, color, 0}; // BR
    v[3] = {x0, y1, 0, 0, color, 0}; // BL
}

void ScreenRenderComponent::draw_rect(const Mat4& model, const Rect& localRect, Color color)
{
    Vec4 corner[4];
    corner[0] = model * Vec4(localRect.x, localRect.y, 0.0f, 1.0f);
    corner[1] = model * Vec4(localRect.x + localRect.w, localRect.y, 0.0f, 1.0f);
    corner[2] = model * Vec4(localRect.x + localRect.w, localRect.y + localRect.h, 0.0f, 1.0f);
    corner[3] = model * Vec4(localRect.x, localRect.y + localRect.h, 0.0f, 1.0f);

    QuadVertex* v = mObj->mRectBatch.write_quad();
    v[0] = {corner[0].x, corner[0].y, 0, 0, color, 0}; // TL
    v[1] = {corner[1].x, corner[1].y, 0, 0, color, 0}; // TR
    v[2] = {corner[2].x, corner[2].y, 0, 0, color, 0}; // BR
    v[3] = {corner[3].x, corner[3].y, 0, 0, color, 0}; // BL
}

void ScreenRenderComponent::draw_rect_outline(const Rect& rect, Color color, float thickness)
{
    if (mObj->mRectBatch.get_quad_count() + 4 > mObj->mRectBatch.get_max_rect_count())
        mObj->flush_rects();

    color = color * mObj->mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    Rect barT(x0, y0, rect.w, thickness);
    Rect barB(x0, y1 - thickness, rect.w, thickness);
    Rect barL(x0, y0 + thickness, thickness, rect.h - 2 * thickness);
    Rect barR(x1 - thickness, y0 + thickness, thickness, rect.h - 2 * thickness);

    draw_rect(barT, color);
    draw_rect(barB, color);
    draw_rect(barL, color);
    draw_rect(barR, color);
}

void ScreenRenderComponent::draw_rect_outline(const Mat4& model, const Rect& localRect, Color color, float thickness)
{
    float x0 = localRect.x;
    float x1 = localRect.x + localRect.w;
    float y0 = localRect.y;
    float y1 = localRect.y + localRect.h;

    Rect barT(x0, y0, localRect.w, thickness);
    Rect barB(x0, y1 - thickness, localRect.w, thickness);
    Rect barL(x0, y0 + thickness, thickness, localRect.h - 2 * thickness);
    Rect barR(x1 - thickness, y0 + thickness, thickness, localRect.h - 2 * thickness);

    draw_rect(model, barT, color);
    draw_rect(model, barB, color);
    draw_rect(model, barL, color);
    draw_rect(model, barR, color);
}

void ScreenRenderComponent::draw_rect_rounded(const Rect& rect, Color color, float radius)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    const Rect uv{0.0f, 0.0f, 1.0f, 1.0f};
    mObj->write_quad_rounded(rect, color, 0, uv, radius);
}

void ScreenRenderComponent::draw_ellipse(const Rect& rect, Color color)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    color = color * mObj->mColorMask;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    UVec4 control = get_quad_vertex_control_bits(0, QUAD_MODE_ELLIPSE, 0.0f, 0.0f);

    QuadVertex* v = mObj->mRectBatch.write_quad();
    v[0] = {x0, y0, 0.0f, 0.0f, color, control[0]}; // TL
    v[1] = {x1, y0, 0.0f, 0.0f, color, control[1]}; // TR
    v[2] = {x1, y1, 0.0f, 0.0f, color, control[2]}; // BR
    v[3] = {x0, y1, 0.0f, 0.0f, color, control[3]}; // BL
}

void ScreenRenderComponent::draw_ellipse_image(const Rect& rect, Color color, RImage image, const Rect& uv)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    mObj->write_quad_ellipse(rect, color, imageIdx, uv);
}

void ScreenRenderComponent::draw_image(const Rect& rect, Color color, RImage image, const Rect& uv, bool forceAlphaOne)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    mObj->write_quad_image(rect, color, imageIdx, uv, forceAlphaOne);
}

void ScreenRenderComponent::draw_image_rounded(const Rect& rect, Color color, RImage image, const Rect& uv, float radius)
{
    if (mObj->mRectBatch.is_full())
        mObj->flush_rects();

    int imageIdx = mObj->get_image_index(image);
    LD_ASSERT(imageIdx >= 0);

    mObj->write_quad_rounded(rect, color, imageIdx, uv, radius);
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

    QuadMode mode = QUAD_MODE_NONE;

    switch (atlas.type())
    {
    case FONT_ATLAS_BITMAP:
        mode = QUAD_MODE_FONT;
        break;
    case FONT_ATLAS_SDF:
        mode = QUAD_MODE_FONT_SDF;
        break;
    }

    color = color * mObj->mColorMask;

    UVec4 control = get_quad_vertex_control_bits(imageIdx, mode, filterRatio / 32.0f, 0.0f);

    QuadVertex* v = mObj->mRectBatch.write_quad();
    v[0] = {x0, y0, u0, v0, color, control[0]}; // TL
    v[1] = {x1, y0, u1, v0, color, control[1]}; // TR
    v[2] = {x1, y1, u1, v1, color, control[2]}; // BR
    v[3] = {x0, y1, u0, v1, color, control[3]}; // BL
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
