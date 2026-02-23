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
layout (location = 1) out flat uint vColor;
layout (location = 2) out flat uint vControl;
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
    vColor = aColor;
    vControl = aControl;
}
)";

// hard coded in fragment shader
static_assert(QuadPipeline::image_slots() == 8);

static const char sQuadFS[] = R"(
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
        case 3: // override image alpha with 1.0
            color.a = 1.0;
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
};

QuadPipeline QuadPipeline::create(RDevice device)
{
    QuadPipelineObj* obj = heap_new<QuadPipelineObj>(MEMORY_USAGE_RENDER);

    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sQuadVS});
    obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sQuadFS});

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

} // namespace LD