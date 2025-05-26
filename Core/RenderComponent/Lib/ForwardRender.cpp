#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/ForwardRender.h>
#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/Pipeline/LinePipeline.h>
#include <vector>

namespace LD {

constexpr uint32_t sMaxPointVertexCount = 512;

struct ForwardRenderComponentObj
{
    /// @brief for host mapped memory, we need duplicates per frame in flight
    struct Frame
    {
        std::vector<RBuffer> pointVBOs;
    };

    RDevice device;
    RCommandList list;
    RSet frameSet;
    RPipeline meshPipeline;
    LinePipeline linePipeline;
    PointVertexBatch<sMaxPointVertexCount> pointBatch;
    ForwardRenderComponent::RenderCallback callback;
    void* user;
    std::vector<Frame> frames;
    uint32_t frameIdx;
    uint32_t batchIdx;
    bool hasInit;
    bool isDrawScope;

    void init(RDevice device);

    void flush_lines();

    void draw_mesh_ex(RCommandList list, RMesh mesh, const Mat4& transform);

    static void on_release(void* user);
    static void on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData);
} sFRCompObj;

void ForwardRenderComponentObj::init(RDevice device)
{
    if (hasInit)
        return;

    this->device = device;
    hasInit = true;
    linePipeline = LinePipeline::create(device);
    frames.resize(device.get_frames_in_flight_count());

    for (Frame& frame : frames)
    {
        RBufferInfo bufferI = {
            .usage = RBUFFER_USAGE_VERTEX_BIT,
            .size = sizeof(RectVertex) * pointBatch.get_point_capacity(),
            .hostVisible = true, // persistent mapping
        };
        frame.pointVBOs = {device.create_buffer(bufferI)};
        frame.pointVBOs[0].map();
    }

    RGraph::add_release_callback(this, &ForwardRenderComponentObj::on_release);
}

void ForwardRenderComponentObj::draw_mesh_ex(RCommandList list, RMesh mesh, const Mat4& transform)
{
    list.cmd_bind_vertex_buffers(0, 1, &mesh.vbo);
    list.cmd_bind_index_buffer(mesh.ibo, RINDEX_TYPE_U32);
    list.cmd_bind_graphics_pipeline(meshPipeline);

    int matIdx = -1;

    for (uint32_t i = 0; i < mesh.primCount; i++)
    {
        const RMeshPrimitive& prim = mesh.prims[i];
        RMaterial* mat = mesh.mats + prim.matIndex;

        if (matIdx != (int)prim.matIndex)
        {
            list.cmd_bind_graphics_sets(sRMeshPipelineLayout, 1, 1, &mat->set);
            matIdx = (int)prim.matIndex;
        }

        RDrawIndexedInfo drawI{};
        drawI.indexCount = prim.indexCount;
        drawI.indexStart = prim.indexStart;
        drawI.instanceCount = 1;
        drawI.instanceStart = 0;
        list.cmd_draw_indexed(drawI);
    }
}

void ForwardRenderComponentObj::flush_lines()
{
    Frame& frame = frames[frameIdx];

    uint32_t pointCount = pointBatch.get_point_count();
    uint32_t vertexCount;
    PointVertex* vertices = pointBatch.get_vertices(vertexCount);
    frame.pointVBOs[batchIdx].map_write(0, sizeof(PointVertex) * vertexCount, vertices);

    pointBatch.reset();
    list.cmd_bind_vertex_buffers(0, 1, frame.pointVBOs.data() + batchIdx);

    list.cmd_bind_graphics_pipeline(linePipeline.handle());

    RDrawInfo drawI{
        .vertexCount = pointCount,
        .vertexStart = 0,
        .instanceCount = 1,
        .instanceStart = 0,
    };
    list.cmd_draw(drawI);

    if (++batchIdx < frame.pointVBOs.size())
        return;

    RBufferInfo bufferI = {
        .usage = RBUFFER_USAGE_VERTEX_BIT,
        .size = sizeof(PointVertex) * pointBatch.get_point_capacity(),
        .hostVisible = true, // persistent mapping
    };

    frame.pointVBOs.push_back(device.create_buffer(bufferI));
    frame.pointVBOs.back().map();
}

void ForwardRenderComponentObj::on_release(void* user)
{
    ForwardRenderComponentObj* compObj = (ForwardRenderComponentObj*)user;
    RDevice device = compObj->device;

    for (Frame& frame : compObj->frames)
    {
        for (RBuffer vbo : frame.pointVBOs)
        {
            vbo.unmap();
            device.destroy_buffer(vbo);
        }

        frame.pointVBOs.clear();
    }

    LinePipeline::destroy(compObj->linePipeline);
}

void ForwardRenderComponentObj::on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData)
{
    ForwardRenderComponentObj* compObj = (ForwardRenderComponentObj*)userData;

    list.cmd_bind_graphics_sets(sRMeshPipelineLayout, 0, 1, &compObj->frameSet);

    compObj->list = list;
    compObj->isDrawScope = true;
    compObj->callback({compObj}, compObj->user);
    compObj->flush_lines();
    compObj->isDrawScope = false;
}

ForwardRenderComponent ForwardRenderComponent::add(RGraph graph, const ForwardRenderComponentInfo& componentI, RSet frameSet, RenderCallback callback, void* user)
{
    LD_PROFILE_SCOPE;

    ForwardRenderComponentObj* compObj = &sFRCompObj;
    RDevice device = graph.get_device();
    compObj->init(device);
    compObj->frameIdx = device.get_frame_index();
    compObj->batchIdx = 0;
    compObj->callback = callback;
    compObj->user = user;
    compObj->frameSet = frameSet;
    compObj->meshPipeline = {};
    compObj->pointBatch.reset();

    RSamplerInfo colorSampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    RSamplerInfo idSampler = {RFILTER_NEAREST, RFILTER_NEAREST, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};

    ForwardRenderComponent forwardComp(compObj);
    RComponent comp = graph.add_component(forwardComp.component_name());
    comp.add_output_image(forwardComp.color_name(), componentI.cFormat, componentI.width, componentI.height, &colorSampler);
    comp.add_output_image(forwardComp.id_color_name(), RFORMAT_RGBA8U, componentI.width, componentI.height, &idSampler);
    comp.add_output_image(forwardComp.depth_stencil_name(), componentI.dsFormat, componentI.width, componentI.height);

    RGraphicsPassInfo gpI{};
    gpI.name = forwardComp.component_name();
    gpI.width = componentI.width;
    gpI.height = componentI.height;

    RClearColorValue idClearColor = RUtil::make_clear_color<uint32_t>(0, 0, 0, 0);
    RGraphicsPass pass = comp.add_graphics_pass(gpI, compObj, &ForwardRenderComponentObj::on_graphics_pass);
    pass.use_color_attachment(forwardComp.color_name(), RATTACHMENT_LOAD_OP_CLEAR, &componentI.clearColor);
    pass.use_color_attachment(forwardComp.id_color_name(), RATTACHMENT_LOAD_OP_CLEAR, &idClearColor);
    pass.use_depth_stencil_attachment(forwardComp.depth_stencil_name(), RATTACHMENT_LOAD_OP_CLEAR, &componentI.clearDS);

    return forwardComp;
}

void ForwardRenderComponent::set_mesh_pipeline(RPipeline meshPipeline)
{
    LD_ASSERT(mObj->isDrawScope);

    mObj->meshPipeline = meshPipeline;
}

void ForwardRenderComponent::draw_mesh(RMesh mesh, const Mat4& transform, uint16_t id)
{
    LD_ASSERT(mObj->isDrawScope);
    LD_ASSERT(mObj->meshPipeline);

    RCommandList list = mObj->list;

    // render Color and 16-bit ID
    mObj->meshPipeline.set_color_write_mask(0, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT | RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
    mObj->meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT);
    mObj->meshPipeline.set_depth_test_enable(true);

    TVec2<uint32_t> idflags;
    idflags.x = id;
    idflags.y = 0;
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(Mat4), &transform);
    list.cmd_push_constant(sRMeshPipelineLayout, sizeof(Mat4), sizeof(idflags), &idflags);

    mObj->draw_mesh_ex(list, mesh, transform);
}

void ForwardRenderComponent::draw_mesh_outline_flags(RMesh mesh, const Mat4& transform)
{
    LD_ASSERT(mObj->isDrawScope);
    LD_ASSERT(mObj->meshPipeline);

    RCommandList list = mObj->list;

    // render 16-bit flags
    mObj->meshPipeline.set_color_write_mask(0, 0);
    mObj->meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
    mObj->meshPipeline.set_depth_test_enable(false);

    TVec2<uint32_t> idflags;
    idflags.x = 0;
    idflags.y = 1; // currently any non-zero flag value indicates mesh that requires outlining
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(Mat4), &transform);
    list.cmd_push_constant(sRMeshPipelineLayout, sizeof(Mat4), sizeof(idflags), &idflags);

    mObj->draw_mesh_ex(list, mesh, transform);
}

void ForwardRenderComponent::draw_line(const Vec3& p0, const Vec3& p1, uint32_t color)
{
    LD_ASSERT(mObj->isDrawScope);

    if (mObj->pointBatch.get_point_count() + 2 >= mObj->pointBatch.get_point_capacity())
        mObj->flush_lines();

    mObj->pointBatch.write_line(p0, p1, color);
}

void ForwardRenderComponent::draw_aabb_outline(const Vec3& min, const Vec3& max, uint32_t color)
{
    LD_ASSERT(mObj->isDrawScope);

    if (mObj->pointBatch.get_point_count() + 24 >= mObj->pointBatch.get_point_capacity())
        mObj->flush_lines();

    Vec3 p0 = {min.x, min.y, min.z};
    Vec3 p1 = {max.x, min.y, min.z};
    Vec3 p2 = {min.x, min.y, max.z};
    Vec3 p3 = {max.x, min.y, max.z};

    Vec3 p4 = {min.x, max.y, min.z};
    Vec3 p5 = {max.x, max.y, min.z};
    Vec3 p6 = {min.x, max.y, max.z};
    Vec3 p7 = {max.x, max.y, max.z};

    draw_line(p0, p1, color);
    draw_line(p0, p2, color);
    draw_line(p1, p3, color);
    draw_line(p2, p3, color);

    draw_line(p4, p5, color);
    draw_line(p4, p6, color);
    draw_line(p5, p7, color);
    draw_line(p6, p7, color);

    draw_line(p0, p4, color);
    draw_line(p1, p5, color);
    draw_line(p2, p6, color);
    draw_line(p3, p7, color);
}

} // namespace LD
