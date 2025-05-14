#include <Ludens/Header/GLSL/Common.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/RMaterial.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/Pipeline/RMeshPipeline.h>
#include <Ludens/System/Memory.h>
#include <vector>

namespace LD {

// clang-format off
static const char sBlinnPhongVS[] = R"(
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 0) out vec3 vPos;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec2 vUV;
)"
LD_GLSL_FRAME_SET
R"(

layout (push_constant) uniform PC {
    mat4 model;
} uPC;

void main()
{
    vec4 worldPos = uPC.model * vec4(aPos, 1.0);
    gl_Position = uFrame.viewProjMat * worldPos;
    mat3 normalMat = transpose(inverse(mat3(uPC.model)));

    vPos = worldPos.xyz;
    vNormal = normalize(normalMat * aNormal);
    vUV = aUV;
}
)";

static const char sBlinnPhongFS[] = R"(
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 0) out vec4 fColor;
#define M_PI 3.1415926535
)"
LD_GLSL_FRAME_SET
LD_GLSL_MATERIAL_SET(1)
LD_GLSL_ROTATE
LD_GLSL_GET_NORMAL
R"(
void main()
{
    vec3 lightDir = normalize(vec3(uFrame.dirLight));
    vec3 viewDir = normalize(uFrame.viewPos.xyz);
    vec3 H = normalize(lightDir + viewDir);
    vec3 N = vNormal;
    vec4 mrSample = texture(uMatMetallicRoughness, vUV);
    vec3 color = uMat.colorFactor.rgb;

    float metallic = 0.0;
    float roughness = 0.0;

    if (uMat.hasColorTexture > 0)
        color = texture(uMatColor, vUV).rgb;

    if (uMat.hasNormalTexture > 0)
        N = get_normal(vPos, vNormal, vUV, texture(uMatNormal, vUV).rgb);

    if (uMat.hasMetallicRoughnessTexture > 0)
        roughness = mrSample.g * uMat.roughnessFactor;    

    if (uMat.hasMetallicRoughnessTexture > 0)
        metallic = mrSample.b * uMat.metallicFactor;

    vec3 envN = ld_rotate(uFrame.envPhase * 2.0 * M_PI, vec3(0.0, -1.0, 0.0)) * N;
    vec3 env = texture(uEnv, envN).rgb;

    color = mix(env, color, roughness);

    vec3 ambient = color * 0.2;
    vec3 diffuse = color * 0.4 * max(dot(lightDir, N), 0.0);
    vec3 specular = color * 0.4 * pow(max(dot(H, N), 0.0), 5.0);

    fColor = vec4(ambient + diffuse + specular, 1.0);
}
)";
// clang-format on

struct RMeshBlinnPhongPipelineObj
{
    RDevice device;         /// the device used to create this pipeline
    RPipeline handle;       /// graphics pipeline handle
    RShader vertexShader;   /// blinn phong vertex shadser
    RShader fragmentShader; /// blinn phong fragment shader
};

RMeshBlinnPhongPipeline RMeshBlinnPhongPipeline::create(RDevice device)
{
    RMeshBlinnPhongPipelineObj* obj = (RMeshBlinnPhongPipelineObj*)heap_malloc(sizeof(RMeshBlinnPhongPipelineObj), MEMORY_USAGE_RENDER);

    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sBlinnPhongVS});
    obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sBlinnPhongFS});

    std::vector<RVertexAttribute> attrs;
    RVertexBinding binding{RBINDING_INPUT_RATE_VERTEX, sizeof(MeshVertex)};
    get_mesh_vertex_attributes(attrs);

    RShader shaders[2] = {obj->vertexShader, obj->fragmentShader};
    RPipelineBlendState blendAttachment = RUtil::make_default_blend_state();
    RPipelineInfo pipelineI{
        .shaderCount = 2,
        .shaders = shaders,
        .vertexAttributeCount = (uint32_t)attrs.size(),
        .vertexAttributes = attrs.data(),
        .vertexBindingCount = 1,
        .vertexBindings = &binding,
        .layout = sRMeshPipelineLayout,
        .rasterization = {
            .polygonMode = RPOLYGON_MODE_FILL,
            .cullMode = RCULL_MODE_NONE,
        },
        .depthStencil = {
            .depthTestEnabled = true,
            .depthWriteEnabled = true,
            .depthCompareOp = RCOMPARE_OP_LESS,
        },
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendAttachment,
        },
    };

    obj->handle = device.create_pipeline(pipelineI);

    return {obj};
}

void RMeshBlinnPhongPipeline::destroy(RMeshBlinnPhongPipeline pipeline)
{
    RMeshBlinnPhongPipelineObj* obj = pipeline;
    RDevice device = obj->device;

    device.destroy_pipeline(obj->handle);
    device.destroy_shader(obj->vertexShader);
    device.destroy_shader(obj->fragmentShader);

    heap_free(obj);
}

RPipeline RMeshBlinnPhongPipeline::handle()
{
    return mObj->handle;
}

} // namespace LD