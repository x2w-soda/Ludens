#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/DualKawase.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <array>
#include <vector>

#define MIP_COUNT 2

namespace LD {

static const char sScreenVS[] = R"(
layout (location = 0) out vec2 vUV;

// embedded attributes
const float attrs[24] = float[](
    -1.0, -1.0, 0.0, 0.0,
    +1.0, -1.0, 1.0, 0.0,
    +1.0, +1.0, 1.0, 1.0,
    +1.0, +1.0, 1.0, 1.0,
    -1.0, +1.0, 0.0, 1.0,
    -1.0, -1.0, 0.0, 0.0
);

void main()
{
    float x = attrs[4 * gl_VertexIndex + 0];
    float y = attrs[4 * gl_VertexIndex + 1];
    float u = attrs[4 * gl_VertexIndex + 2];
    float v = attrs[4 * gl_VertexIndex + 3];
    gl_Position = vec4(vec2(x, y), 0.0, 1.0);
    vUV = vec2(u, v);
}
)";

static const char sDownSampleFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 fColor;

layout (set = 1, binding = 0) uniform sampler2D uImage;

void main()
{
    vec2 uvStep = vec2(1.0) / textureSize(uImage, 0);
    float halfPixelU = uvStep.x * 0.5;
    float halfPixelV = uvStep.y * 0.5;
    float kernelSize = 10.0;

    vec4 color = texture(uImage, vUV) * 4.0;
    color += texture(uImage, vUV + vec2(+halfPixelU, +halfPixelV) * kernelSize);
    color += texture(uImage, vUV + vec2(+halfPixelU, -halfPixelV) * kernelSize);
    color += texture(uImage, vUV + vec2(-halfPixelU, +halfPixelV) * kernelSize);
    color += texture(uImage, vUV + vec2(-halfPixelU, -halfPixelV) * kernelSize);
    fColor = color / 8.0;
}
)";

static const char sUpSampleFS[] = R"(
layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 fColor;

layout (set = 1, binding = 0) uniform sampler2D uImage;

void main()
{
    vec2 uvStep = vec2(1.0) / textureSize(uImage, 0);
    float halfPixelU = uvStep.x * 0.5;
    float halfPixelV = uvStep.y * 0.5;
    float kernelSize = 6.0;

    vec4 color = texture(uImage, vUV + vec2(-halfPixelU * 2.0, 0.0) * kernelSize);
    color += texture(uImage, vUV + vec2(-halfPixelU, +halfPixelV)   * kernelSize) * 2.0;
    color += texture(uImage, vUV + vec2(0.0, +halfPixelV * 2.0)     * kernelSize);
    color += texture(uImage, vUV + vec2(+halfPixelU, +halfPixelV)   * kernelSize) * 2.0;
    color += texture(uImage, vUV + vec2(+halfPixelU * 2.0, 0.0)     * kernelSize);
    color += texture(uImage, vUV + vec2(+halfPixelU, -halfPixelV)   * kernelSize) * 2.0;
    color += texture(uImage, vUV + vec2(0.0, -halfPixelV * 2.0)     * kernelSize);
    color += texture(uImage, vUV + vec2(-halfPixelU, -halfPixelV)   * kernelSize) * 2.0;
    fColor = color / 12.0;
}
)";

struct DualKawaseComponentObj
{
    struct Frame
    {
        RBuffer vbo;
        RSetPool blurSetPool;
        std::array<RImage, MIP_COUNT + 1> blurImages;
        std::array<RSet, MIP_COUNT + 1> blurSets;
    };

    RDevice device;
    RShader screenVS;
    RShader downSampleFS;
    RShader upSampleFS;
    RPipelineLayoutInfo blurPipelineLayout;
    RPipeline downSamplePipeline;
    RPipeline upSamplePipeline;
    std::vector<Frame> frames;
    uint32_t mipLevel;
    uint32_t frameIdx;
    uint32_t baseWidth;
    uint32_t baseHeight;
    bool hasInit;

    void init(RDevice device, RFormat format, uint32_t width, uint32_t height);

    void invalidate_image(Frame& frame, uint32_t idx, RImage handle);

    static void on_release(void* user);
    static void on_down_sample(RGraphicsPass pass, RCommandList list, void* user);
    static void on_up_sample(RGraphicsPass pass, RCommandList list, void* user);

} sDKCompObj;

void DualKawaseComponentObj::init(RDevice device, RFormat format, uint32_t width, uint32_t height)
{
    if (hasInit)
        return;

    this->device = device;
    hasInit = true;
    baseWidth = width;
    baseHeight = height;

    screenVS = device.create_shader({RSHADER_TYPE_VERTEX, sScreenVS});
    downSampleFS = device.create_shader({RSHADER_TYPE_FRAGMENT, sDownSampleFS});
    upSampleFS = device.create_shader({RSHADER_TYPE_FRAGMENT, sUpSampleFS});

    static RSetBindingInfo blurSetImage{0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, 1};
    static RSetLayoutInfo blurSetLayout{.bindingCount = 1, .bindings = &blurSetImage};
    static RSetLayoutInfo setLayouts[2]{sFrameSetLayout, blurSetLayout};

    blurPipelineLayout.setLayoutCount = 2;
    blurPipelineLayout.setLayouts = setLayouts;

    RShader shaders[2] = {screenVS, downSampleFS};
    RPipelineBlendState blendState = RUtil::make_default_blend_state();
    RPipelineInfo pipelineI = {
        .shaderCount = 2,
        .shaders = shaders,
        .vertexAttributeCount = 0,
        .vertexBindingCount = 0,
        .layout = blurPipelineLayout,
        .depthStencil = {
            .depthTestEnabled = false,
        },
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendState,
        },
    };
    downSamplePipeline = device.create_pipeline(pipelineI);

    shaders[1] = upSampleFS;
    upSamplePipeline = device.create_pipeline(pipelineI);

    const RSamplerInfo sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    uint32_t mipWidth = baseWidth;
    uint32_t mipHeight = baseHeight;

    std::array<RSetImageUpdateInfo, MIP_COUNT + 1> updates;

    frames.resize(device.get_frames_in_flight_count());

    for (Frame& frame : frames)
    {
        RSetPoolInfo poolI;
        poolI.layout = blurSetLayout;
        poolI.maxSets = MIP_COUNT + 1;
        frame.blurSetPool = device.create_set_pool(poolI);

        for (int i = 0; i <= MIP_COUNT; i++)
        {
            frame.blurSets[i] = frame.blurSetPool.allocate();
            frame.blurImages[i] = {}; // unknown until graphics pass
        }
    }

    RGraph::add_release_callback(this, &DualKawaseComponentObj::on_release);
}

void DualKawaseComponentObj::on_release(void* user)
{
    DualKawaseComponentObj* obj = (DualKawaseComponentObj*)user;
    RDevice device = obj->device;

    for (Frame& frame : obj->frames)
    {
        device.destroy_set_pool(frame.blurSetPool);
    }

    device.destroy_pipeline(obj->upSamplePipeline);
    device.destroy_pipeline(obj->downSamplePipeline);
    device.destroy_shader(obj->downSampleFS);
    device.destroy_shader(obj->upSampleFS);
    device.destroy_shader(obj->screenVS);
}

void DualKawaseComponentObj::invalidate_image(Frame& frame, uint32_t idx, RImage handle)
{
    LD_PROFILE_SCOPE;

    RImageLayout imageLayout = RIMAGE_LAYOUT_SHADER_READ_ONLY;

    // TODO: checking with (frame.blurImages[idx] != handle) is broken?
    //       is RGraphicsPass::get_image returning out-of-date handles?

    frame.blurImages[idx] = handle;
    RSetImageUpdateInfo update = RUtil::make_single_set_image_update_info(
        frame.blurSets[idx], 0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, &handle);
    
    device.update_set_images(1, &update);
}

void DualKawaseComponentObj::on_down_sample(RGraphicsPass pass, RCommandList list, void* user)
{
    LD_PROFILE_SCOPE;

    DualKawaseComponentObj* obj = (DualKawaseComponentObj*)user;
    Frame& frame = obj->frames[obj->frameIdx];

    uint32_t mipLevel = obj->mipLevel++;
    std::string mipName = "mip_0";
    mipName.back() = '0' + mipLevel;

    RSet blurSet = {};
    RImage sampled = {};
    RImageLayout imageLayout = RIMAGE_LAYOUT_SHADER_READ_ONLY;

    if (mipLevel == 0) // sample from component input
    {
        blurSet = frame.blurSets[MIP_COUNT];
        sampled = pass.get_image(DualKawaseComponent(obj).input_name());
        obj->invalidate_image(frame, MIP_COUNT, sampled);
    }
    else // sample from lower mip level
    {
        blurSet = frame.blurSets[mipLevel - 1];
        mipName.back()--;
        sampled = pass.get_image(mipName.c_str());
        obj->invalidate_image(frame, mipLevel - 1, sampled);
    }

    list.cmd_bind_graphics_pipeline(obj->downSamplePipeline);
    list.cmd_bind_graphics_sets(obj->blurPipelineLayout, 1, 1, &blurSet);
    list.cmd_draw({.vertexCount = 6, .vertexStart = 0, .instanceCount = 1, .instanceStart = 0});
}

void DualKawaseComponentObj::on_up_sample(RGraphicsPass pass, RCommandList list, void* user)
{
    LD_PROFILE_SCOPE;

    DualKawaseComponentObj* obj = (DualKawaseComponentObj*)user;
    Frame& frame = obj->frames[obj->frameIdx];

    LD_ASSERT(obj->mipLevel != 0);

    uint32_t mipLevel = --obj->mipLevel;
    RSet blurSet = frame.blurSets[mipLevel];

    if (mipLevel == MIP_COUNT - 1)
    {
        std::string mipName = "mip_0";
        mipName.back() = '0' + MIP_COUNT - 1;
        RImage sampled = pass.get_image(mipName.c_str());
        obj->invalidate_image(frame, MIP_COUNT - 1, sampled);
    }

    list.cmd_bind_graphics_pipeline(obj->upSamplePipeline);
    list.cmd_bind_graphics_sets(obj->blurPipelineLayout, 1, 1, &blurSet);
    list.cmd_draw({.vertexCount = 6, .vertexStart = 0, .instanceCount = 1, .instanceStart = 0});
}

DualKawaseComponent DualKawaseComponent::add(RGraph graph, RFormat format, uint32_t width, uint32_t height)
{
    LD_PROFILE_SCOPE;

    RDevice device = graph.get_device();

    sDKCompObj.init(device, format, width, height);
    sDKCompObj.mipLevel = 0;
    sDKCompObj.frameIdx = device.get_frame_index();

    DualKawaseComponent kawaseComp(&sDKCompObj);
    RSamplerInfo sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};

    RComponent comp = graph.add_component(kawaseComp.component_name());
    comp.add_input_image(kawaseComp.input_name(), format, width, height);
    comp.add_output_image(kawaseComp.output_name(), format, width, height, &sampler);

    uint32_t mipWidth = width;
    uint32_t mipHeight = height;

    std::string passName = "down_sample_0";
    std::string mipName = "mip_0";

    for (int i = 0; i < MIP_COUNT; i++)
    {
        mipWidth /= 2;
        mipHeight /= 2;

        passName.back() = '0' + i;
        mipName.back() = '0' + i;
        comp.add_private_image(mipName.c_str(), format, mipWidth, mipHeight, &sampler);

        RGraphicsPassInfo gpI;
        gpI.width = mipWidth;
        gpI.height = mipHeight;
        gpI.name = passName.c_str();
        RGraphicsPass downSamplePass = comp.add_graphics_pass(gpI, &sDKCompObj, &DualKawaseComponentObj::on_down_sample);

        if (i == 0)
        {
            downSamplePass.use_image_sampled(kawaseComp.input_name());
            downSamplePass.use_color_attachment(mipName.c_str(), RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);
        }
        else
        {
            downSamplePass.use_color_attachment(mipName.c_str(), RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);
            mipName.back()--;
            downSamplePass.use_image_sampled(mipName.c_str());
        }
    }

    passName = "up_sample_0";

    for (int i = MIP_COUNT - 1; i >= 0; i--)
    {
        mipWidth *= 2;
        mipHeight *= 2;

        passName.back() = '0' + i;
        mipName.back() = '0' + i;

        RGraphicsPassInfo gpI;
        gpI.width = mipWidth;
        gpI.height = mipHeight;
        gpI.name = passName.c_str();
        RGraphicsPass upSamplePass = comp.add_graphics_pass(gpI, &sDKCompObj, &DualKawaseComponentObj::on_up_sample);

        if (i == 0)
        {
            upSamplePass.use_image_sampled(mipName.c_str());
            upSamplePass.use_color_attachment(kawaseComp.output_name(), RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);
        }
        else
        {
            upSamplePass.use_image_sampled(mipName.c_str());
            mipName.back()--;
            upSamplePass.use_color_attachment(mipName.c_str(), RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);
        }
    }

    return kawaseComp;
}

} // namespace LD
