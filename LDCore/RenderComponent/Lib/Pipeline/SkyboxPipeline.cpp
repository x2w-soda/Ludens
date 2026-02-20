#include <Ludens/Header/GLSL/Common.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Pipeline/SkyboxPipeline.h>

#include <array>
#include <vector>

namespace LD {

// clang-format off
static const char sSkyboxVS[] = R"(
layout (location = 0) out vec3 vDir;
#define M_PI 3.1415926535
)"
LD_GLSL_FRAME_SET
LD_GLSL_ROTATE
R"(
// embedded position attributes
const float aPos[108] = float[](
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
);

layout (push_constant) uniform PC {
    uint vpIndex;
} uPC;

void main()
{
    float x = aPos[3 * gl_VertexIndex + 0];
    float y = aPos[3 * gl_VertexIndex + 1];
    float z = aPos[3 * gl_VertexIndex + 2];

    ViewProjectionData vp = uFrame.vp[uPC.vpIndex];
    mat3 rotMat = ld_rotate(uFrame.envPhase * 2.0 * M_PI, vec3(0.0, 1.0, 0.0));
    mat4 viewMat = mat4(mat3(vp.viewMat));

    mat4 modelMat = mat4(
        vec4(rotMat[0], 0.0),
        vec4(rotMat[1], 0.0),
        vec4(rotMat[2], 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    vec4 pos = vp.projMat * viewMat * modelMat * vec4(x, y, z, 1.0);

    gl_Position = pos.xyww;
    vDir = vec3(x, y, z);
}
)";

static const char sSkyboxFS[] = R"(
layout (location = 0) in vec3 vDir;
layout (location = 0) out vec4 fColor;
)"
LD_GLSL_FRAME_SET
R"(
void main()
{
    fColor = vec4(texture(uEnv, vDir).rgb, 1.0);
}
)";
// clang-format on

struct SkyboxPipelineObj
{
    RDevice device;         /// the device used to create this pipeline
    RPipeline handle;       /// graphics pipeline handle
    RShader vertexShader;   /// blinn phong vertex shadser
    RShader fragmentShader; /// blinn phong fragment shader
};

SkyboxPipeline SkyboxPipeline::create(RDevice device)
{
    SkyboxPipelineObj* obj = (SkyboxPipelineObj*)heap_malloc(sizeof(SkyboxPipelineObj), MEMORY_USAGE_RENDER);

    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sSkyboxVS});
    obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sSkyboxFS});

    std::array<RPipelineBlendState, 2> blendAttachments;
    std::array<RShader, 2> shaders = {obj->vertexShader, obj->fragmentShader};
    std::array<RPipelineBlendState, 2> blendStates;
    blendStates[0] = RUtil::make_default_blend_state();
    blendStates[1].enabled = false; // TODO: this is the ID-Flags color attachment, parameterize for removal?

    RPipelineLayoutInfo pipelineLayoutI{};
    pipelineLayoutI.setLayoutCount = 1;
    pipelineLayoutI.setLayouts = &sFrameSetLayout;

    RPipelineInfo pipelineI = {
        .shaderCount = (uint32_t)shaders.size(),
        .shaders = shaders.data(),
        .vertexAttributeCount = 0,
        .vertexBindingCount = 0,
        .layout = pipelineLayoutI,
        .rasterization = {
            .polygonMode = RPOLYGON_MODE_FILL,
            .cullMode = RCULL_MODE_NONE,
        },
        .depthStencil = {
            .depthTestEnabled = true, .depthWriteEnabled = false,
            .depthCompareOp = RCOMPARE_OP_LESS_OR_EQUAL, // we will be rendering skybox depth as 1.0, so equality matters
        },
        .blend = {
            .colorAttachmentCount = (uint32_t)blendStates.size(),
            .colorAttachments = blendStates.data(),
        },
    };

    obj->handle = device.create_pipeline(pipelineI);

    return {obj};
}

void SkyboxPipeline::destroy(SkyboxPipeline pipeline)
{
    SkyboxPipelineObj* obj = pipeline;
    RDevice device = obj->device;

    device.destroy_pipeline(obj->handle);
    device.destroy_shader(obj->vertexShader);
    device.destroy_shader(obj->fragmentShader);

    heap_free(obj);
}

RPipeline SkyboxPipeline::handle()
{
    return mObj->handle;
}

} // namespace LD