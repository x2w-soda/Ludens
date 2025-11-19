#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Pipeline/OutlinePipeline.h>
#include <Ludens/System/Memory.h>
#include <vector>

namespace LD {

// clang-format off
static const char sOutlineVS[] = R"(
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 0) out vec2 vUV;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char sOutlineFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 fColor;

// sample from ID color attachment which has RFORMAT_RGBA8U.
// r and g channel describes 16-bit identifier, we don't need this.
// b and a channel describes 16-bit flags, currently non-zero means the mesh needs outlining
layout (set = 1, binding = 0) uniform usampler2D uIDFlags;

uint get_flags(vec2 uv)
{
    uvec4 texel = texture(uIDFlags, uv);
    uint flags = 0;
    flags |= (texel.b & 0xFF);
    flags |= (texel.a & 0xFF) << 8;
    return flags;
}

void main()
{
    vec2 aspect = 1.0 / vec2(textureSize(uIDFlags, 0));
    const float radians = 6.28318530;
    const float steps = 36.0;
    const float radius = 3.0;

    float hits = 0.0;

    uint flags = get_flags(vUV);
    if (flags != 0)
        discard;

    for (float i = 0.0; i < radians; i += radians / steps)
    {
        vec2 uvOffset = vec2(sin(i), cos(i)) * aspect * radius;
        flags = get_flags(vUV + uvOffset);
		
        if (flags > 0)
            hits += 1.0;
    }

    if (hits == 0.0)
        discard;

    float dist = hits / steps;
    float alpha = smoothstep(0.05, 0.15, dist);
    fColor = vec4(1.0, 156.0/255.0, 28.0/255.0, alpha);
}
)";
// clang-format on

static RSetLayoutInfo sOutlineSetLayouts[2]{
    sFrameSetLayout,
    sSingleSampleSetLayout,
};

static RPipelineLayoutInfo sOutlinePipelineLayout{
    .setLayoutCount = 2,
    .setLayouts = sOutlineSetLayouts,
};

struct OutlinePipelineObj
{
    RDevice device;         /// the device used to create this pipeline
    RPipeline handle;       /// graphics pipeline handle
    RShader vertexShader;   /// outline vertex shadser
    RShader fragmentShader; /// outline fragment shader
};

OutlinePipeline OutlinePipeline::create(RDevice device)
{
    OutlinePipelineObj* obj = (OutlinePipelineObj*)heap_malloc(sizeof(OutlinePipelineObj), MEMORY_USAGE_RENDER);

    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sOutlineVS});
    obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sOutlineFS});
    RShader shaders[2] = {obj->vertexShader, obj->fragmentShader};

    RPipelineBlendState blendState = RUtil::make_default_blend_state();
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
        .layout = sOutlinePipelineLayout,
        .rasterization = {
            .polygonMode = RPOLYGON_MODE_FILL,
            .cullMode = RCULL_MODE_NONE,
        },
        .depthStencil = {
            .depthTestEnabled = false,
        },
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendState,
        },
    };
    obj->handle = device.create_pipeline(pipelineI);

    return OutlinePipeline(obj);
}

void OutlinePipeline::destroy(OutlinePipeline pipeline)
{
    OutlinePipelineObj* obj = pipeline;
    RDevice device = obj->device;

    device.destroy_pipeline(obj->handle);
    device.destroy_shader(obj->fragmentShader);
    device.destroy_shader(obj->vertexShader);

    heap_free(obj);

    device = {};
}

RPipelineLayoutInfo OutlinePipeline::get_layout()
{
    return sOutlinePipelineLayout;
}

RPipeline OutlinePipeline::handle()
{
    return mObj->handle;
}

} // namespace LD