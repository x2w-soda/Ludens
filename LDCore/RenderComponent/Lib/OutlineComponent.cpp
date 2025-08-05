#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Outline.h>
#include <Ludens/RenderComponent/Pipeline/OutlinePipeline.h>
#include <Ludens/System/Memory.h>
#include <string>
#include <unordered_map>

namespace LD {

static std::unordered_map<std::string, OutlineComponentObj*> sComponents;
static OutlinePipeline sOutlinePipeline;
static RDevice sDevice;
static RBuffer sScreenVBO;
static int sComponentCtr = 0;

struct OutlineComponentObj
{
    struct Frame
    {
        RSet set;
    };

    RDevice device;
    RPipeline pipeline;
    RSetPool setPool;
    std::string name;
    std::vector<Frame> frames;

    OutlineComponentObj(RDevice device);
    ~OutlineComponentObj();

    static void init(RDevice device);
    static void on_release(void* user);
    static void on_destroy(void* user);

    static void on_graphics_pass(RGraphicsPass pass, RCommandList list, void* user);
};

OutlineComponentObj::OutlineComponentObj(RDevice device)
{
    this->device = device;

    if (!sOutlinePipeline)
    {
        sOutlinePipeline = OutlinePipeline::create(device);
        pipeline = sOutlinePipeline.handle();
    }

    LD_ASSERT(pipeline);

    // TODO: probably not one pool per component instance
    RSetPoolInfo poolI;
    poolI.layout = sSingleSampleSetLayout;
    poolI.maxSets = 2;
    setPool = device.create_set_pool(poolI);
    frames.resize(device.get_frames_in_flight_count());
    for (Frame& frame : frames)
        frame.set = setPool.allocate();
}

OutlineComponentObj::~OutlineComponentObj()
{
    device.destroy_set_pool(setPool);
}

/// @brief static initialization
void OutlineComponentObj::init(RDevice device)
{
    if (sDevice)
        return;

    sDevice = device;

    RGraph::add_release_callback(nullptr, &OutlineComponentObj::on_release);

    // clang-format off
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        +1.0f, -1.0f, 1.0f, 0.0f,
        +1.0f, +1.0f, 1.0f, 1.0f,
        +1.0f, +1.0f, 1.0f, 1.0f,
        -1.0f, +1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
    };
    // clang-format on

    sScreenVBO = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vertices), false});
    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    stager.add_buffer_data(sScreenVBO, vertices);
    stager.submit(device.get_graphics_queue());
}

/// @brief static shutdown
void OutlineComponentObj::on_release(void* user)
{
    sDevice.destroy_buffer(sScreenVBO);

    if (sOutlinePipeline)
        OutlinePipeline::destroy(sOutlinePipeline);

    for (auto& ite : sComponents)
    {
        OutlineComponentObj* obj = ite.second;

        heap_delete<OutlineComponentObj>(obj);
    }

    sComponents.clear();
    sDevice = {};
}

void OutlineComponentObj::on_destroy(void* user)
{
    sComponentCtr = 0;
}

void OutlineComponentObj::on_graphics_pass(RGraphicsPass pass, RCommandList list, void* user)
{
    OutlineComponentObj* obj = (OutlineComponentObj*)user;
    OutlineComponentObj::Frame& frame = obj->frames[obj->device.get_frame_index()];

    RImageLayout layout;
    RImage image = pass.get_image(OutlineComponent(obj).input_name(), &layout);
    RSetImageUpdateInfo updateI = RUtil::make_single_set_image_update_info(frame.set, 0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &image);
    obj->device.update_set_images(1, &updateI);

    static RPipelineLayoutInfo sOutlinePipelineLayout = OutlinePipeline::get_layout();
    list.cmd_bind_graphics_sets(sOutlinePipelineLayout, 1, 1, &frame.set);
    list.cmd_bind_vertex_buffers(0, 1, &sScreenVBO);
    list.cmd_bind_graphics_pipeline(obj->pipeline);
    list.cmd_draw({.vertexCount = 6, .vertexStart = 0, .instanceCount = 1, .instanceStart = 0});
}

OutlineComponent OutlineComponent::add(RGraph& graph, const OutlineComponentInfo& info)
{
    RDevice device = graph.get_device();
    OutlineComponentObj::init(device);
    OutlineComponentObj* obj = nullptr;

    LD_ASSERT(sComponentCtr == 0); // currently a singleton

    std::string name = "outline_" + std::to_string(sComponentCtr++);
    RGraph::add_destroy_callback(nullptr, &OutlineComponentObj::on_destroy); // TODO: on_new_frame callback?

    if (sComponents.contains(name))
    {
        obj = sComponents[name];
    }
    else
    {
        obj = heap_new<OutlineComponentObj>(MEMORY_USAGE_RENDER, device);
        obj->name = name;
        sComponents[name] = obj;
    }

    OutlineComponent outlineComp(obj);

    RComponent comp = graph.add_component(name.c_str());
    comp.add_io_image(outlineComp.io_name(), info.format, info.width, info.height);
    comp.add_input_image(outlineComp.input_name(), info.format, info.width, info.height);

    RGraphicsPassInfo gpI;
    gpI.width = info.width;
    gpI.height = info.height;
    gpI.name = name.c_str();
    RGraphicsPass pass = comp.add_graphics_pass(gpI, obj, &OutlineComponentObj::on_graphics_pass);
    pass.use_color_attachment(outlineComp.io_name(), RATTACHMENT_LOAD_OP_LOAD, nullptr);
    pass.use_image_sampled(outlineComp.input_name());

    return outlineComp;
}

const char* OutlineComponent::component_name() const
{
    return mObj->name.c_str();
}

} // namespace LD