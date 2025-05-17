#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/ScreenPick.h>
#include <array>
#include <vector>

#define MAX_QUERY_COUNT 8

namespace LD {

struct PickQuery
{
    uint32_t posx;
    uint32_t posy;
    uint32_t result;
    uint32_t pad;
};

static_assert(sizeof(PickQuery) == 16);
static_assert(alignof(PickQuery) == 4);

// clang-format off
static const char sScreenPickCS[] = R"(
layout (local_size_x = 8) in;

layout (set = 1, binding = 0, rgba8ui) readonly uniform uimage2D sImage;

struct PickQuery
{
    uvec2 pos;   // picking position
    uint result; // picking result
    uint pad;    // padding for array alignment
};

layout (set = 1, binding = 1, std430) buffer QueryBuffer {
    PickQuery queries[];
} sQueryBuffer;

void main()
{
    uint i = uint(gl_GlobalInvocationID.x);

    uint result = 0;
    uvec4 texel = imageLoad(sImage, ivec2(sQueryBuffer.queries[i].pos));
    result |= (texel.r & 0xFF);
    result |= (texel.g & 0xFF) << 8;
    result |= (texel.b & 0xFF) << 16;
    result |= (texel.a & 0xFF) << 24;
    sQueryBuffer.queries[i].result = result;
}
)";
// clang-format on

struct ScreenPickComponentObj
{
    struct Frame
    {
        RBuffer querySSBO;
        RSet querySet;
        uint32_t queryCount;
        uint32_t resultCount;
        PickQuery queries[MAX_QUERY_COUNT];
        PickQuery results[MAX_QUERY_COUNT];
    };

    RDevice device;
    RPipeline pipeline;
    RShader shader;
    RSetPool setPool;
    RPipelineLayoutInfo pipelineLI;
    uint32_t frameIdx;
    std::vector<Frame> frames;

    void init(RDevice device);

    static void on_release(void* user);
    static void on_compute_pass(RComputePass pass, RCommandList list, void* userData);
} sCompObj;

void ScreenPickComponentObj::init(RDevice device)
{
    if (this->device)
        return;

    this->device = device;

    shader = device.create_shader({RSHADER_TYPE_COMPUTE, sScreenPickCS});

    static std::array<RSetBindingInfo, 2> querySetBindings;
    querySetBindings[0] = {0, RBINDING_TYPE_STORAGE_IMAGE, 1};
    querySetBindings[1] = {1, RBINDING_TYPE_STORAGE_BUFFER, 1};

    RSetLayoutInfo querySetLayout{
        .bindingCount = (uint32_t)querySetBindings.size(),
        .bindings = querySetBindings.data(),
    };

    static std::array<RSetLayoutInfo, 2> setLayouts;
    setLayouts[0] = sFrameSetLayout;
    setLayouts[1] = querySetLayout;

    pipelineLI.setLayoutCount = (uint32_t)setLayouts.size();
    pipelineLI.setLayouts = setLayouts.data();

    RComputePipelineInfo pipelineI{};
    pipelineI.layout = pipelineLI;
    pipelineI.shader = shader;
    pipeline = device.create_compute_pipeline(pipelineI);

    uint32_t framesInFlight = device.get_frames_in_flight_count();

    RSetPoolInfo setPoolI{};
    setPoolI.maxSets = framesInFlight;
    setPoolI.layout = querySetLayout;
    setPool = device.create_set_pool(setPoolI);

    frames.resize(framesInFlight);

    for (Frame& frame : frames)
    {
        frame.querySet = setPool.allocate();
        frame.querySSBO = device.create_buffer({RBUFFER_USAGE_STORAGE_BIT, sizeof(PickQuery) * MAX_QUERY_COUNT, true});
        frame.querySSBO.map();
    }

    RGraph::add_release_callback(&sCompObj, &ScreenPickComponentObj::on_release);
}

void ScreenPickComponentObj::on_release(void* user)
{
    ScreenPickComponentObj* obj = (ScreenPickComponentObj*)user;
    RDevice device = obj->device;

    for (Frame& frame : obj->frames)
    {
        frame.querySSBO.unmap();
        device.destroy_buffer(frame.querySSBO);
    }

    device.destroy_set_pool(obj->setPool);
    device.destroy_pipeline(obj->pipeline);
    device.destroy_shader(obj->shader);
}

void ScreenPickComponentObj::on_compute_pass(RComputePass pass, RCommandList list, void* userData)
{
    ScreenPickComponentObj* obj = (ScreenPickComponentObj*)userData;
    Frame& frame = obj->frames[obj->frameIdx];

    RImage input = pass.get_image(ScreenPickComponent(obj).input_name());

    RImageLayout layout = RIMAGE_LAYOUT_GENERAL;
    RSetImageUpdateInfo imageUI = RUtil::make_single_set_image_update_info(frame.querySet, 0, RBINDING_TYPE_STORAGE_IMAGE, &layout, &input);
    obj->device.update_set_images(1, &imageUI);

    RSetBufferUpdateInfo bufferUI = RUtil::make_single_set_buffer_udpate_info(frame.querySet, 1, RBINDING_TYPE_STORAGE_BUFFER, &frame.querySSBO);
    obj->device.update_set_buffers(1, &bufferUI);

    list.cmd_bind_compute_sets(obj->pipelineLI, 1, 1, &frame.querySet);
    list.cmd_bind_compute_pipeline(obj->pipeline);
    list.cmd_dispatch(1, 1, 1);
}

ScreenPickComponent ScreenPickComponent::add(RGraph graph, const ScreenPickComponentInfo& componentI)
{
    RDevice device = graph.get_device();

    sCompObj.init(device);
    sCompObj.frameIdx = device.get_frame_index();

    ScreenPickComponentObj::Frame& frame = sCompObj.frames[sCompObj.frameIdx];

    // download completed queries
    if (frame.queryCount > 0)
    {
        frame.resultCount = frame.queryCount;

        size_t resultByteSize = sizeof(PickQuery) * frame.resultCount;
        memcpy(frame.results, frame.querySSBO.map_read(0, resultByteSize), resultByteSize);
    }

    if (componentI.pickQueryCount > MAX_QUERY_COUNT)
        printf("ScreenPickComponent: %d queries requested but only %d queries are supported in each frame.\n", (int)componentI.pickQueryCount, MAX_QUERY_COUNT);
    frame.queryCount = std::min(componentI.pickQueryCount, (uint32_t)MAX_QUERY_COUNT);

    // upload new queries
    for (uint32_t i = 0; i < frame.queryCount; i++)
    {
        frame.queries[i].posx = (uint32_t)componentI.pickPositions[i].x;
        frame.queries[i].posy = (uint32_t)componentI.pickPositions[i].y;
        frame.queries[i].result = 0;
    }
    frame.querySSBO.map_write(0, sizeof(PickQuery) * frame.queryCount, frame.queries);

    ScreenPickComponent pickComp(&sCompObj);

    RComponent comp = graph.add_component(pickComp.component_name());
    comp.add_input_image(pickComp.input_name(), RFORMAT_RGBA8U, componentI.width, componentI.height);

    RComputePassInfo cpI{};
    cpI.name = pickComp.component_name();
    RComputePass pass = comp.add_compute_pass(cpI, &sCompObj, &ScreenPickComponentObj::on_compute_pass);
    pass.use_image_storage_read_only(pickComp.input_name());

    return pickComp;
}

void ScreenPickComponent::get_results(std::vector<ScreenPickResult>& results)
{
    ScreenPickComponentObj::Frame& frame = mObj->frames[mObj->frameIdx];

    results.resize(frame.resultCount);
    for (uint32_t i = 0; i < frame.resultCount; i++)
    {
        results[i].pos.x = (float)frame.results[i].posx;
        results[i].pos.y = (float)frame.results[i].posy;
        results[i].id = frame.results[i].result & 0xFFFF;
        results[i].flags = (frame.results[i].result >> 16) & 0xFFFF;
    }

    frame.resultCount = 0;
}

} // namespace LD