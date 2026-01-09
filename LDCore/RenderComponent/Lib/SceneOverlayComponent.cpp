#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Model.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Embed/GizmoMesh.h>
#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Pipeline/OutlinePipeline.h>
#include <Ludens/RenderComponent/Pipeline/RMeshPipeline.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>

#include <string>
#include <unordered_map>

namespace LD {

struct CopyPipelineObj
{
    RDevice device;
    RShader vertexShader;
    RShader fragmentShader;
    RPipeline handle;
};

static RSetLayoutInfo sCopyPipelineSetLayouts[2] = {
    sFrameSetLayout,
    sDoubleSampleSetLayout,
};

static RPipelineLayoutInfo sCopyPipelineLayout{
    .setLayoutCount = 2,
    .setLayouts = sCopyPipelineSetLayouts,
};

static const char sCopyVS[] = R"(
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 0) out vec2 vUV;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char sCopyFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 fColor;
layout (location = 1) out uvec4 fIDFlags;

layout (set = 1, binding = 0) uniform sampler2D uColor;
layout (set = 1, binding = 1) uniform usampler2D uIDFlags;

void main()
{
    fColor = texture(uColor, vUV);
    fIDFlags = texture(uIDFlags, vUV);
}
)";

/// @brief Intermediate pipeline used to copy existing color values to the multi-sampled color attachments.
///        This is required if we wish to render the Gizmos with MSAA.
struct CopyPipeline : Handle<CopyPipelineObj>
{
    static CopyPipeline create(RDevice device)
    {
        CopyPipelineObj* obj = (CopyPipelineObj*)heap_malloc(sizeof(CopyPipelineObj), MEMORY_USAGE_RENDER);

        obj->device = device;
        obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sCopyVS});
        obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sCopyFS});
        RShader shaders[2] = {obj->vertexShader, obj->fragmentShader};

        RPipelineBlendState blendStates[2];
        blendStates[0].enabled = false;
        blendStates[1].enabled = false;
        std::vector<RVertexAttribute> attrs{
            {GLSL_TYPE_VEC2, 0, 0},                 // aPos
            {GLSL_TYPE_VEC2, sizeof(float) * 2, 0}, // aUV
        };
        RVertexBinding binding{RBINDING_INPUT_RATE_VERTEX, sizeof(float) * 4};

        RPipelineInfo pipelineI{
            .shaderCount = 2,
            .shaders = shaders,
            .vertexAttributeCount = (uint32_t)attrs.size(),
            .vertexAttributes = attrs.data(),
            .vertexBindingCount = 1,
            .vertexBindings = &binding,
            .primitiveTopology = RPRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .layout = sCopyPipelineLayout,
            .rasterization = {
                .polygonMode = RPOLYGON_MODE_FILL,
                .cullMode = RCULL_MODE_NONE,
            },
            .depthStencil = {
                .depthTestEnabled = false,
            },
            .blend = {
                .colorAttachmentCount = 2,
                .colorAttachments = blendStates,
            },
        };
        obj->handle = device.create_pipeline(pipelineI);

        return CopyPipeline(obj);
    }

    static void destroy(CopyPipeline pipeline)
    {
        CopyPipelineObj* obj = pipeline;
        RDevice device = obj->device;

        device.destroy_pipeline(obj->handle);
        device.destroy_shader(obj->vertexShader);
        device.destroy_shader(obj->fragmentShader);

        heap_free(obj);
    }

    inline RPipeline handle()
    {
        return mObj->handle;
    }
};

static std::unordered_map<std::string, SceneOverlayComponentObj*> sComponents;
static CopyPipeline sCopyPipeline;
static OutlinePipeline sOutlinePipeline;
static RMeshAmbientPipeline sMeshPipeline;
static RDevice sDevice;
static RBuffer sScreenVBO;
static RBuffer sTranslationGizmoVBO;
static RBuffer sTranslationGizmoIBO;
static RBuffer sRotationGizmoVBO;
static RBuffer sRotationGizmoIBO;
static RBuffer sPlaneXY;
static RBuffer sPlaneXZ;
static RBuffer sPlaneYZ;
static RBuffer sScaleGizmoVBO;
static RBuffer sScaleGizmoIBO;
static int sComponentCtr = 0;

struct SceneOverlayComponentObj
{
    struct Frame
    {
        RSet outlineSet; // used during outline pass
        RSet gizmoSet;   // used during gizmo pass
    };

    RDevice device;
    RPipeline outlinePipeline;
    RPipeline meshPipeline;
    RPipeline copyPipeline;
    RSetPool outlineSetPool;
    RSetPool gizmoSetPool;
    Vec3 gizmoCenter;
    Color gizmoColorX;
    Color gizmoColorY;
    Color gizmoColorZ;
    Color gizmoColorXY;
    Color gizmoColorXZ;
    Color gizmoColorYZ;
    SceneOverlayGizmo gizmoType;
    float gizmoScale;
    std::string name;
    std::vector<Frame> frames;

    SceneOverlayComponentObj(RDevice device);
    ~SceneOverlayComponentObj();

    static void init(RDevice device);
    static void on_release(void* user);
    static void on_destroy(void* user);

    static void on_outline_graphics_pass(RGraphicsPass pass, RCommandList list, void* user);
    static void on_gizmo_graphics_pass(RGraphicsPass pass, RCommandList list, void* user);

    void draw_translation_gizmo(RGraphicsPass pass, RCommandList list);
    void draw_rotation_gizmo(RGraphicsPass pass, RCommandList list);
    void draw_scale_gizmo(RGraphicsPass pass, RCommandList list);
};

SceneOverlayComponentObj::SceneOverlayComponentObj(RDevice device)
{
    this->device = device;

    if (!sCopyPipeline)
    {
        sCopyPipeline = CopyPipeline::create(device);
        copyPipeline = sCopyPipeline.handle();
    }

    if (!sOutlinePipeline)
    {
        sOutlinePipeline = OutlinePipeline::create(device);
        outlinePipeline = sOutlinePipeline.handle();
    }

    if (!sMeshPipeline)
    {
        sMeshPipeline = RMeshAmbientPipeline::create(device);
        meshPipeline = sMeshPipeline.handle();
    }

    LD_ASSERT(outlinePipeline);
    LD_ASSERT(meshPipeline);

    const uint32_t framesInFlightCount = device.get_frames_in_flight_count();

    RSetPoolInfo poolI;
    poolI.layout = sSingleSampleSetLayout;
    poolI.maxSets = framesInFlightCount;
    outlineSetPool = device.create_set_pool(poolI);

    poolI.layout = sDoubleSampleSetLayout;
    gizmoSetPool = device.create_set_pool(poolI);

    frames.resize(framesInFlightCount);
    for (Frame& frame : frames)
    {
        frame.outlineSet = outlineSetPool.allocate();
        frame.gizmoSet = gizmoSetPool.allocate();
    }
}

SceneOverlayComponentObj::~SceneOverlayComponentObj()
{
    device.destroy_set_pool(gizmoSetPool);
    device.destroy_set_pool(outlineSetPool);
}

/// @brief static initialization
void SceneOverlayComponentObj::init(RDevice device)
{
    if (sDevice)
        return;

    sDevice = device;

    RGraph::add_release_callback(nullptr, &SceneOverlayComponentObj::on_release);

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

    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    sScreenVBO = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vertices), false});
    stager.add_buffer_data(sScreenVBO, vertices);

    const MeshVertex* gizmoVertices;
    const uint32_t* gizmoIndices;
    uint32_t vertexCount, indexCount;

    EmbeddedGizmoMesh::get_translation_gizmo_axis(&gizmoVertices, vertexCount, &gizmoIndices, indexCount);
    sTranslationGizmoVBO = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(MeshVertex) * vertexCount, false});
    sTranslationGizmoIBO = device.create_buffer({RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(uint32_t) * indexCount, false});
    stager.add_buffer_data(sTranslationGizmoVBO, gizmoVertices);
    stager.add_buffer_data(sTranslationGizmoIBO, gizmoIndices);

    EmbeddedGizmoMesh::get_rotation_gizmo_plane(&gizmoVertices, vertexCount, &gizmoIndices, indexCount);
    sRotationGizmoVBO = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(MeshVertex) * vertexCount, false});
    sRotationGizmoIBO = device.create_buffer({RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(uint32_t) * indexCount, false});
    stager.add_buffer_data(sRotationGizmoVBO, gizmoVertices);
    stager.add_buffer_data(sRotationGizmoIBO, gizmoIndices);

    EmbeddedGizmoMesh::get_scale_gizmo_axis(&gizmoVertices, vertexCount, &gizmoIndices, indexCount);
    sScaleGizmoVBO = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(MeshVertex) * vertexCount, false});
    sScaleGizmoIBO = device.create_buffer({RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(uint32_t) * indexCount, false});
    stager.add_buffer_data(sScaleGizmoVBO, gizmoVertices);
    stager.add_buffer_data(sScaleGizmoIBO, gizmoIndices);

    EmbeddedGizmoMesh::get_gizmo_plane_xy(&gizmoVertices, vertexCount);
    sPlaneXY = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(MeshVertex) * vertexCount, false});
    stager.add_buffer_data(sPlaneXY, gizmoVertices);

    EmbeddedGizmoMesh::get_gizmo_plane_xz(&gizmoVertices, vertexCount);
    sPlaneXZ = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(MeshVertex) * vertexCount, false});
    stager.add_buffer_data(sPlaneXZ, gizmoVertices);

    EmbeddedGizmoMesh::get_gizmo_plane_yz(&gizmoVertices, vertexCount);
    sPlaneYZ = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(MeshVertex) * vertexCount, false});
    stager.add_buffer_data(sPlaneYZ, gizmoVertices);

    stager.submit(device.get_graphics_queue());
}

/// @brief static shutdown
void SceneOverlayComponentObj::on_release(void* user)
{
    sDevice.destroy_buffer(sPlaneYZ);
    sDevice.destroy_buffer(sPlaneXZ);
    sDevice.destroy_buffer(sPlaneXY);
    sDevice.destroy_buffer(sScaleGizmoVBO);
    sDevice.destroy_buffer(sScaleGizmoIBO);
    sDevice.destroy_buffer(sRotationGizmoVBO);
    sDevice.destroy_buffer(sRotationGizmoIBO);
    sDevice.destroy_buffer(sTranslationGizmoVBO);
    sDevice.destroy_buffer(sTranslationGizmoIBO);
    sDevice.destroy_buffer(sScreenVBO);

    if (sOutlinePipeline)
        OutlinePipeline::destroy(sOutlinePipeline);

    if (sMeshPipeline)
        RMeshAmbientPipeline::destroy(sMeshPipeline);

    if (sCopyPipeline)
        CopyPipeline::destroy(sCopyPipeline);

    for (auto& ite : sComponents)
    {
        SceneOverlayComponentObj* obj = ite.second;

        heap_delete<SceneOverlayComponentObj>(obj);
    }

    sComponents.clear();
    sDevice = {};
}

void SceneOverlayComponentObj::on_destroy(void* user)
{
    sComponentCtr = 0;
}

/// @brief Perform outlining based on the 16-bit flags in the id-flags attachment.
///        Renders to scene color attachment. Samples from id-flag attachment.
void SceneOverlayComponentObj::on_outline_graphics_pass(RGraphicsPass pass, RCommandList list, void* user)
{
    SceneOverlayComponentObj* obj = (SceneOverlayComponentObj*)user;
    SceneOverlayComponentObj::Frame& frame = obj->frames[obj->device.get_frame_index()];

    RImageLayout layout;
    RImage image = pass.get_image(SceneOverlayComponent(obj).in_idflags_name(), &layout);
    RSetImageUpdateInfo updateI = RUtil::make_single_set_image_update_info(frame.outlineSet, 0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &image);
    obj->device.update_set_images(1, &updateI);

    static RPipelineLayoutInfo sOutlinePipelineLayout = OutlinePipeline::get_layout();
    list.cmd_bind_graphics_sets(sOutlinePipelineLayout, 1, 1, &frame.outlineSet);
    list.cmd_bind_vertex_buffers(0, 1, &sScreenVBO);
    list.cmd_bind_graphics_pipeline(obj->outlinePipeline);
    list.cmd_draw({.vertexCount = 6, .instanceCount = 1, .vertexStart = 0, .instanceStart = 0});
}

/// @brief Render Gizmo in world space.
void SceneOverlayComponentObj::on_gizmo_graphics_pass(RGraphicsPass pass, RCommandList list, void* user)
{
    SceneOverlayComponentObj* obj = (SceneOverlayComponentObj*)user;
    SceneOverlayComponentObj::Frame& frame = obj->frames[obj->device.get_frame_index()];

    // inputColorImage should already have outlines drawn, we now render Gizmos on top with MSAA
    RImageLayout layout;
    RImage inputColorImage = pass.get_image(SceneOverlayComponent(obj).in_color_name(), &layout);
    RImage inputIDFlagsImage = pass.get_image(SceneOverlayComponent(obj).in_idflags_name(), &layout);
    RSetImageUpdateInfo updates[2];
    updates[0] = RUtil::make_single_set_image_update_info(frame.gizmoSet, 0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &inputColorImage);
    updates[1] = RUtil::make_single_set_image_update_info(frame.gizmoSet, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &inputIDFlagsImage);
    obj->device.update_set_images(2, updates);

    // Copy input color attachments to output color attachments
    // that are potentially multi-sampled.
    list.cmd_bind_graphics_pipeline(obj->copyPipeline);
    list.cmd_bind_graphics_sets(sCopyPipelineLayout, 1, 1, &frame.gizmoSet);
    list.cmd_bind_vertex_buffers(0, 1, &sScreenVBO);
    list.cmd_draw({.vertexCount = 6, .instanceCount = 1, .vertexStart = 0, .instanceStart = 0});

    // render gizmo mesh with ambient shading
    // write color and write 16-bit ID
    list.cmd_bind_graphics_pipeline(obj->meshPipeline);
    obj->meshPipeline.set_depth_test_enable(false);
    obj->meshPipeline.set_color_write_mask(0, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT | RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
    obj->meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT);

    switch (obj->gizmoType)
    {
    case SCENE_OVERLAY_GIZMO_TRANSLATION:
        obj->draw_translation_gizmo(pass, list);
        break;
    case SCENE_OVERLAY_GIZMO_ROTATION:
        obj->draw_rotation_gizmo(pass, list);
        break;
    case SCENE_OVERLAY_GIZMO_SCALE:
        obj->draw_scale_gizmo(pass, list);
        break;
    case SCENE_OVERLAY_GIZMO_NONE:
    default:
        break;
    }
}

void SceneOverlayComponentObj::draw_translation_gizmo(RGraphicsPass pass, RCommandList list)
{
    list.cmd_bind_vertex_buffers(0, 1, &sTranslationGizmoVBO);
    list.cmd_bind_index_buffer(sTranslationGizmoIBO, RINDEX_TYPE_U32);

    RDrawIndexedInfo drawI;
    EmbeddedGizmoMesh::get_translation_gizmo_axis_draw_info(drawI);

    Mat4 translation = Mat4::translate(gizmoCenter);
    Mat4 scale = Mat4::scale(Vec3(gizmoScale));

    RMeshAmbientPipeline::PushConstant pc;
    pc.flags = 0;
    pc.model = translation * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_AXIS_X;
    pc.ambient = gizmoColorX.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    pc.model = translation * Mat4::rotate(M_PI_2, Vec3(0.0f, 0.0f, 1.0f)) * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_AXIS_Y;
    pc.ambient = gizmoColorY.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    pc.model = translation * Mat4::rotate(M_PI_2, Vec3(0.0f, -1.0f, 0.0f)) * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_AXIS_Z;
    pc.ambient = gizmoColorZ.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    RDrawInfo planeDrawI;
    EmbeddedGizmoMesh::get_gizmo_plane_draw_info(planeDrawI);

    float offset = 0.15f * gizmoScale;
    Mat4 planeScale = Mat4::scale(Vec3(0.3f)) * scale;
    Mat4 planeOffset = Mat4::translate(Vec3(offset, offset, 0.0f));

    pc.model = translation * planeOffset * planeScale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_PLANE_XY;
    pc.ambient = gizmoColorXY.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_bind_vertex_buffers(0, 1, &sPlaneXY);
    list.cmd_draw(planeDrawI);

    planeOffset = Mat4::translate(Vec3(offset, 0.0f, offset));

    pc.model = translation * planeOffset * planeScale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_PLANE_XZ;
    pc.ambient = gizmoColorXZ.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_bind_vertex_buffers(0, 1, &sPlaneXZ);
    list.cmd_draw(planeDrawI);

    planeOffset = Mat4::translate(Vec3(0.0f, offset, offset));

    pc.model = translation * planeOffset * planeScale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_PLANE_YZ;
    pc.ambient = gizmoColorYZ.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_bind_vertex_buffers(0, 1, &sPlaneYZ);
    list.cmd_draw(planeDrawI);
}

void SceneOverlayComponentObj::draw_rotation_gizmo(RGraphicsPass pass, RCommandList list)
{
    list.cmd_bind_vertex_buffers(0, 1, &sRotationGizmoVBO);
    list.cmd_bind_index_buffer(sRotationGizmoIBO, RINDEX_TYPE_U32);

    RDrawIndexedInfo drawI;
    EmbeddedGizmoMesh::get_rotation_gizmo_plane_draw_info(drawI);

    Mat4 translation = Mat4::translate(gizmoCenter);
    Mat4 scale = Mat4::scale(Vec3(gizmoScale));

    RMeshAmbientPipeline::PushConstant pc;
    pc.model = translation * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_PLANE_XZ;
    pc.flags = 0;
    pc.ambient = gizmoColorXZ.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    pc.model = translation * Mat4::rotate(M_PI_2, Vec3(-1.0f, 0.0f, 0.0f)) * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_PLANE_XY;
    pc.ambient = gizmoColorXY.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    pc.model = translation * Mat4::rotate(M_PI_2, Vec3(0.0f, 0.0f, 1.0f)) * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_PLANE_YZ;
    pc.ambient = gizmoColorYZ.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);
}

void SceneOverlayComponentObj::draw_scale_gizmo(RGraphicsPass pass, RCommandList list)
{
    list.cmd_bind_vertex_buffers(0, 1, &sScaleGizmoVBO);
    list.cmd_bind_index_buffer(sScaleGizmoIBO, RINDEX_TYPE_U32);

    RDrawIndexedInfo drawI;
    EmbeddedGizmoMesh::get_scale_gizmo_axis_draw_info(drawI);

    Mat4 translation = Mat4::translate(gizmoCenter);
    Mat4 scale = Mat4::scale(Vec3(gizmoScale));

    RMeshAmbientPipeline::PushConstant pc;
    pc.model = translation * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_AXIS_X;
    pc.flags = 0;
    pc.ambient = gizmoColorX.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    pc.model = translation * Mat4::rotate(M_PI_2, Vec3(0.0f, 0.0f, 1.0f)) * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_AXIS_Y;
    pc.ambient = gizmoColorY.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);

    pc.model = translation * Mat4::rotate(M_PI_2, Vec3(0.0f, -1.0f, 0.0f)) * scale;
    pc.id = SCENE_OVERLAY_GIZMO_ID_AXIS_Z;
    pc.ambient = gizmoColorZ.as_vec4();
    list.cmd_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
    list.cmd_draw_indexed(drawI);
}

SceneOverlayComponent SceneOverlayComponent::add(RGraph& graph, const SceneOverlayComponentInfo& info)
{
    RDevice device = graph.get_device();
    SceneOverlayComponentObj::init(device);
    SceneOverlayComponentObj* obj = nullptr;

    LD_ASSERT(sComponentCtr == 0); // currently a singleton

    std::string name = "sceneoverlay" + std::to_string(sComponentCtr++);
    RGraph::add_destroy_callback(nullptr, &SceneOverlayComponentObj::on_destroy); // TODO: on_new_frame callback?

    if (sComponents.contains(name))
    {
        obj = sComponents[name];
    }
    else
    {
        obj = heap_new<SceneOverlayComponentObj>(MEMORY_USAGE_RENDER, device);
        obj->name = name;
        sComponents[name] = obj;
    }

    obj->gizmoCenter = info.gizmoCenter;
    obj->gizmoType = info.gizmoType;
    obj->gizmoScale = info.gizmoScale;
    obj->gizmoColorX = info.gizmoColorX;
    obj->gizmoColorY = info.gizmoColorY;
    obj->gizmoColorZ = info.gizmoColorZ;
    obj->gizmoColorXY = info.gizmoColorXY;
    obj->gizmoColorXZ = info.gizmoColorXZ;
    obj->gizmoColorYZ = info.gizmoColorYZ;
    SceneOverlayComponent overlayComp(obj);

    RComponent comp = graph.add_component(name.c_str());
    comp.add_input_image(overlayComp.in_color_name(), info.colorFormat, info.width, info.height);
    comp.add_input_image(overlayComp.in_idflags_name(), RFORMAT_RGBA8U, info.width, info.height);
    comp.add_output_image(overlayComp.out_color_name(), info.colorFormat, info.width, info.height);
    comp.add_output_image(overlayComp.out_idflags_name(), RFORMAT_RGBA8U, info.width, info.height);

    // Draw outline on top of input scene color, the input ID-flags is sampled to determine
    // the silhouette of the screen-space outlining algorithm.
    std::string outlineGPName = name + "outline";
    RGraphicsPassInfo gpI{};
    gpI.width = info.width;
    gpI.height = info.height;
    gpI.name = outlineGPName.c_str();
    gpI.samples = RSAMPLE_COUNT_1_BIT;
    RGraphicsPass outlineGP = comp.add_graphics_pass(gpI, obj, &SceneOverlayComponentObj::on_outline_graphics_pass);
    outlineGP.use_color_attachment(overlayComp.in_color_name(), RATTACHMENT_LOAD_OP_LOAD, nullptr);
    outlineGP.use_image_sampled(overlayComp.in_idflags_name());

    // Draw anti-aliased Gizmos with MSAA. We first copy input scene color and ID-flags to multi-sampled
    // color attachments before drawing some Gizmos on top of outlines.
    std::string gizmoGPName = name + "gizmo";
    gpI.name = gizmoGPName.c_str();
    gpI.samples = info.gizmoMSAA;
    RGraphicsPass gizmoGP = comp.add_graphics_pass(gpI, obj, &SceneOverlayComponentObj::on_gizmo_graphics_pass);
    gizmoGP.use_color_attachment(overlayComp.out_color_name(), RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);
    gizmoGP.use_color_attachment(overlayComp.out_idflags_name(), RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);
    gizmoGP.use_image_sampled(overlayComp.in_color_name());
    gizmoGP.use_image_sampled(overlayComp.in_idflags_name());

    return overlayComp;
}

const char* SceneOverlayComponent::component_name() const
{
    return mObj->name.c_str();
}

} // namespace LD