#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/Pipeline/QuadPipeline.h>

#define IMAGE_SLOT_COUNT 8

namespace LD {

static RSetBindingInfo sScreenSetBinding = {0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, IMAGE_SLOT_COUNT};
static RSetLayoutInfo sImageSlotSetLayout = {.bindingCount = 1, .bindings = &sScreenSetBinding};

static Array<RSetLayoutInfo, 2> sQuadPipelineSetLayouts{
    sFrameSetLayout,
    sImageSlotSetLayout};

static RPipelineLayoutInfo sQuadPipelineLayout = {
    .setLayoutCount = sQuadPipelineSetLayouts.size(),
    .setLayouts = sQuadPipelineSetLayouts.data()};

// clang-format off
static const char sQuadVS[] = R"(
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in uint aColor;
layout (location = 3) in uint aControl;

layout (location = 0) out vec2 vUV;
layout (location = 1) out vec2 vLocalUV;
layout (location = 2) out vec4 vColor;
layout (location = 3) out flat uint vControl;
)"
LD_GLSL_FRAME_SET
R"(

layout (push_constant) uniform PC {
    uint vpIndex;
} uPC;

void main()
{
    ViewProjectionData vp = uFrame.vp[uPC.vpIndex];
    gl_Position = vp.viewProjMat * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
    vLocalUV.x = float(aControl & 1u);
    vLocalUV.y = float((aControl >> 1) & 1u);
    vColor.r = float((aColor >> 24) & 0xFF) / 255.0;
    vColor.g = float((aColor >> 16) & 0xFF) / 255.0;
    vColor.b = float((aColor >> 8) & 0xFF) / 255.0;
    vColor.a = float(aColor & 0xFF) / 255.0;
    vControl = aControl >> 2; // only the first two bits vary across 4 vertices
}
)";

// hard coded in fragment shader
static_assert(QuadPipeline::image_slots() == 8);

static const char sQuadRectFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 1) in vec2 vLocalUV;
layout (location = 2) in vec4 vColor;
layout (location = 3) in flat uint vControl;

layout (location = 0) out vec4 fColor;

layout (set = 1, binding = 0) uniform sampler2D uImages[8];

void main()
{
    vec4 imageColor = vec4(1.0);
    uint imageIdx = vControl & 15;

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

    fColor = imageColor * vColor;
}
)";

static const char sQuadUberFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 1) in vec2 vLocalUV;
layout (location = 2) in vec4 vColor;
layout (location = 3) in flat uint vControl;

layout (location = 0) out vec4 fColor;

layout (set = 1, binding = 0) uniform sampler2D uImages[8];

void main()
{
    vec4 imageColor = vec4(1.0);

    uint imageIdx = vControl & 15;
    uint mode = (vControl >> 4) & 15;
    float r1 = float((vControl >> 8) & 1023) / 1023.0;
    float r2 = float((vControl >> 18) & 1023) / 1023.0;
    float filterRatio = r1 * 32.0;
    float aspectRatio = mix(0.1, 10.0, r1);

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

    float screenPxRange = 2.0 * filterRatio;
    float sd = imageColor.r;
    float screenPxDistance = screenPxRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    
    vec4 color = imageColor * vColor;
    vec2 centerOffset = vLocalUV - vec2(0.5);
    float distSQ;
    float dist;
    float delta;

    switch (mode)
    {
    case 1: // single channel font bitmap
        color = vColor * vec4(imageColor.r);
        break;
    case 2: // font SDF
        color = mix(vec4(0.0), vColor, opacity);
        break;
    case 3: // override image alpha with 1.0
        color.a = 1.0;
        break;
    case 4: // ellipse
        distSQ = dot(centerOffset, centerOffset);
        delta = fwidth(distSQ);
        color.a = 1.0 - smoothstep(0.25 - delta, 0.25 + delta, distSQ);
        break;
    case 5: // rect border radius, find signed distance to arc
        vec2 aspect = vec2(aspectRatio, 1.0);
        vec2 p = centerOffset * aspect;
        vec2 halfExtent = 0.5 * aspect;
        float borderRadius = r2 * 0.5 * min(aspect.x, aspect.y);
        vec2 q = abs(p) - halfExtent + vec2(borderRadius); // relative to inner rect
        dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - borderRadius;
        delta = fwidth(dist);
        color.a = 1.0 - smoothstep(-delta, delta, dist);
        break;
    }

    fColor = color;
}
)";
// clang-format on

struct QuadPipelineObj
{
    RDevice device;
    RPipeline handle;
    RShader vertexShader;
    RShader fragmentShader;
    QuadPipelineType type;
};

QuadPipeline QuadPipeline::create(RDevice device, QuadPipelineType type)
{
    QuadPipelineObj* obj = heap_new<QuadPipelineObj>(MEMORY_USAGE_RENDER);

    obj->type = type;
    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sQuadVS});

    switch (type)
    {
    case QUAD_PIPELINE_RECT:
        obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sQuadRectFS});
        break;
    case QUAD_PIPELINE_UBER:
    default:
        obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sQuadUberFS});
        break;
    }

    Array<RShader, 2> shaders{obj->vertexShader, obj->fragmentShader};
    Vector<RVertexAttribute> attrs;
    RVertexBinding binding = {.inputRate = RBINDING_INPUT_RATE_VERTEX, .stride = sizeof(QuadVertex)};
    get_quad_vertex_attributes(attrs);

    RPipelineBlendState blendState = RUtil::make_default_blend_state();

    RPipelineInfo pipelineI{
        .shaderCount = (uint32_t)shaders.size(),
        .shaders = shaders.data(),
        .vertexAttributeCount = (uint32_t)attrs.size(),
        .vertexAttributes = attrs.data(),
        .vertexBindingCount = 1,
        .vertexBindings = &binding,
        .layout = sQuadPipelineLayout,
        .depthStencil = {
            .depthTestEnabled = false,
        },
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendState,
        },
    };

    obj->handle = device.create_pipeline(pipelineI);

    return QuadPipeline(obj);
}

void QuadPipeline::destroy(QuadPipeline pipeline)
{
    QuadPipelineObj* obj = pipeline.unwrap();

    obj->device.destroy_pipeline(obj->handle);
    obj->device.destroy_shader(obj->fragmentShader);
    obj->device.destroy_shader(obj->vertexShader);

    heap_delete<QuadPipelineObj>(obj);
}

RSetLayoutInfo QuadPipeline::image_slot_set_layout()
{
    return sImageSlotSetLayout;
}

RPipelineLayoutInfo QuadPipeline::layout()
{
    return sQuadPipelineLayout;
}

RPipeline QuadPipeline::handle()
{
    return mObj->handle;
}

QuadPipelineType QuadPipeline::type()
{
    return mObj->type;
}

} // namespace LD