#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/Pipeline/LinePipeline.h>
#include <Ludens/System/Memory.h>
#include <array>
#include <vector>

namespace LD {

// clang-format off
static const char sLineVS[] = R"(
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aColor;
layout (location = 0) out flat uint vColor;
)"
LD_GLSL_FRAME_SET
R"(

void main()
{
    gl_Position = uFrame.viewProjMat * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char sLineFS[] = R"(
layout (location = 0) in flat uint vColor;
layout (location = 0) out vec4 fColor;
layout (location = 1) out uint fID;
)"
LD_GLSL_FRAME_SET
R"(
void main()
{
    float r = float((vColor >> 24) & 0xFF) / 255.0f;
    float g = float((vColor >> 16) & 0xFF) / 255.0f;
    float b = float((vColor >> 8) & 0xFF) / 255.0f;
    float a = float(vColor & 0xFF) / 255.0f;
    fColor = vec4(r, g, b, a);
}
)";
// clang-format on

struct LinePipelineObj
{
    RDevice device;         /// the device used to create this pipeline
    RPipeline handle;       /// graphics pipeline handle
    RShader vertexShader;   /// line vertex shadser
    RShader fragmentShader; /// line fragment shader
};

LinePipeline LinePipeline::create(RDevice device)
{
    LinePipelineObj* obj = (LinePipelineObj*)heap_malloc(sizeof(LinePipelineObj), MEMORY_USAGE_RENDER);

    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sLineVS});
    obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sLineFS});

    std::vector<RVertexAttribute> attrs;
    RVertexBinding binding{RBINDING_INPUT_RATE_VERTEX, sizeof(PointVertex)};
    get_point_vertex_attributes(attrs);

    RShader shaders[2] = {obj->vertexShader, obj->fragmentShader};
    std::array<RPipelineBlendState, 2> blendAttachment;
    blendAttachment[0] = RUtil::make_default_blend_state();
    blendAttachment[1].enabled = false;
    RPipelineInfo pipelineI{
        .shaderCount = 2,
        .shaders = shaders,
        .vertexAttributeCount = (uint32_t)attrs.size(),
        .vertexAttributes = attrs.data(),
        .vertexBindingCount = 1,
        .vertexBindings = &binding,
        .primitiveTopology = RPRIMITIVE_TOPOLOGY_LINE_LIST,
        .layout = sRMeshPipelineLayout,
        .rasterization = {
            .polygonMode = RPOLYGON_MODE_LINE,
            .cullMode = RCULL_MODE_NONE,
            .lineWidth = 2.0f,
        },
        .depthStencil = {
            .depthTestEnabled = true,
            .depthWriteEnabled = true,
            .depthCompareOp = RCOMPARE_OP_LESS,
        },
        .blend = {
            .colorAttachmentCount = (uint32_t)blendAttachment.size(),
            .colorAttachments = blendAttachment.data(),
        },
    };
    obj->handle = device.create_pipeline(pipelineI);

    return {obj};
}

void LinePipeline::destroy(LinePipeline pipeline)
{
    LinePipelineObj* obj = pipeline;
    RDevice device = obj->device;

    device.destroy_pipeline(obj->handle);
    device.destroy_shader(obj->vertexShader);
    device.destroy_shader(obj->fragmentShader);

    heap_free(obj);
}

RPipeline LinePipeline::handle()
{
    return mObj->handle;
}

} // namespace LD