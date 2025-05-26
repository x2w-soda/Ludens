#include <Ludens/Header/GLSL/Common.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Skybox.h>
#include <array>

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

void main()
{
    float x = aPos[3 * gl_VertexIndex + 0];
    float y = aPos[3 * gl_VertexIndex + 1];
    float z = aPos[3 * gl_VertexIndex + 2];

    mat3 rotMat = ld_rotate(uFrame.envPhase * 2.0 * M_PI, vec3(0.0, 1.0, 0.0));
    mat4 viewMat = mat4(mat3(uFrame.viewMat));

    mat4 modelMat = mat4(
        vec4(rotMat[0], 0.0),
        vec4(rotMat[1], 0.0),
        vec4(rotMat[2], 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    vec4 pos = uFrame.projMat * viewMat * modelMat * vec4(x, y, z, 1.0);

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

struct SkyboxComponentObj
{
    RDevice device;
    RPipeline skyboxPipeline;
    RPipelineLayoutInfo pipelineLayoutI;
    RShader skyboxVS;
    RShader skyboxFS;
    bool hasInit;

    void init(RDevice device);

    static void on_release(void* user);
    static void on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData);
} sSBCompObj;

void SkyboxComponentObj::init(RDevice device)
{
    if (hasInit)
        return;

    hasInit = true;
    this->device = device;

    skyboxVS = device.create_shader({RSHADER_TYPE_VERTEX, sSkyboxVS});
    skyboxFS = device.create_shader({RSHADER_TYPE_FRAGMENT, sSkyboxFS});
    std::array<RShader, 2> shaders{skyboxVS, skyboxFS};

    pipelineLayoutI.setLayoutCount = 1;
    pipelineLayoutI.setLayouts = &sFrameSetLayout;

    RPipelineBlendState blendState = RUtil::make_default_blend_state();
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
            .colorAttachmentCount = 1,
            .colorAttachments = &blendState,
        },
    };

    skyboxPipeline = device.create_pipeline(pipelineI);

    RGraph::add_release_callback(this, &SkyboxComponentObj::on_release);
}

void SkyboxComponentObj::on_release(void* user)
{
    SkyboxComponentObj* obj = (SkyboxComponentObj*)user;

    if (!obj->hasInit)
        return;

    obj->hasInit = false;
    RDevice device = obj->device;

    device.destroy_pipeline(obj->skyboxPipeline);
    device.destroy_shader(obj->skyboxFS);
    device.destroy_shader(obj->skyboxVS);
}

void SkyboxComponentObj::on_graphics_pass(RGraphicsPass pass, RCommandList list, void* userData)
{
    SkyboxComponentObj* obj = (SkyboxComponentObj*)userData;

    list.cmd_bind_graphics_pipeline(obj->skyboxPipeline);

    RDrawInfo drawI{
        .vertexCount = 36,
        .vertexStart = 0,
        .instanceCount = 1,
        .instanceStart = 0,
    };
    list.cmd_draw(drawI);
}

SkyboxComponent SkyboxComponent::add(RGraph graph, RFormat cFormat, RFormat dsFormat, uint32_t width, uint32_t height)
{
    LD_PROFILE_SCOPE;

    SkyboxComponentObj* compObj = &sSBCompObj;
    RDevice device = graph.get_device();

    compObj->init(device);

    SkyboxComponent skyboxComp(compObj);
    RComponent comp = graph.add_component(skyboxComp.component_name());
    comp.add_io_image(skyboxComp.io_color_name(), cFormat, width, height);
    comp.add_io_image(skyboxComp.io_depth_stencil_name(), dsFormat, width, height);

    RGraphicsPassInfo gpI{};
    gpI.name = skyboxComp.component_name();
    gpI.width = width;
    gpI.height = height;

    // render skybox on top of previous content
    RGraphicsPass pass = comp.add_graphics_pass(gpI, compObj, &SkyboxComponentObj::on_graphics_pass);
    pass.use_color_attachment(skyboxComp.io_color_name(), RATTACHMENT_LOAD_OP_LOAD, nullptr);
    pass.use_depth_stencil_attachment(skyboxComp.io_depth_stencil_name(), RATTACHMENT_LOAD_OP_LOAD, nullptr);

    return skyboxComp;
}

} // namespace LD
