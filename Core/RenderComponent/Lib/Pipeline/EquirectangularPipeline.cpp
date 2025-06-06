#include <Ludens/Header/GLSL/Common.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Layout/VertexLayouts.h>
#include <Ludens/RenderComponent/Pipeline/EquirectangularPipeline.h>
#include <Ludens/System/Memory.h>
#include <array>

namespace LD {

struct EquirectangularPipelineObj
{
    RDevice device;         /// the owning device
    RPipeline handle;       /// graphics pipeline handle
    RShader vertexShader;   /// line vertex shadser
    RShader fragmentShader; /// line fragment shader
};

// clang-format off
static const char sEquirectangularVS[] = R"(
layout (location = 0) in vec3 aPos;
layout (location = 0) out vec3 vPos;

layout (push_constant) uniform PC {
    mat4 viewProj;
} uPC;

void main()
{
    gl_Position = uPC.viewProj * vec4(aPos, 1.0);
    vPos = aPos;
}
)";

static const char sEquirectangularFS[] = R"(
layout (location = 0) in vec3 vPos;
layout (location = 0) out vec4 fColor;

layout (set = 0, binding = 0) uniform sampler2D uEquirectangular;
)" 
LD_GLSL_TONE_MAP
R"(
const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 get_uv(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 dir = normalize(vPos);
    vec2 uv = get_uv(dir);
    vec3 color = texture(uEquirectangular, uv).rgb;
    fColor = vec4(tone_map_reinhard(color), 1.0);
}
)";
// clang-format on

static const RPipelineLayoutInfo sEquirectangularPipelineLayout{
    .setLayoutCount = 1,
    .setLayouts = &sSingleSampleSetLayout,
};

EquirectangularPipeline EquirectangularPipeline::create(RDevice device)
{
    EquirectangularPipelineObj* obj = (EquirectangularPipelineObj*)heap_malloc(sizeof(EquirectangularPipelineObj), MEMORY_USAGE_RENDER);

    obj->device = device;
    obj->vertexShader = device.create_shader({RSHADER_TYPE_VERTEX, sEquirectangularVS});
    obj->fragmentShader = device.create_shader({RSHADER_TYPE_FRAGMENT, sEquirectangularFS});

    RVertexAttribute attribute{RGLSL_TYPE_VEC3, 0, 0};
    RVertexBinding binding{RBINDING_INPUT_RATE_VERTEX, sizeof(Vec3)};

    sSingleSampleSetLayout;

    RShader shaders[2] = {obj->vertexShader, obj->fragmentShader};
    RPipelineBlendState blendAttachment = RUtil::make_default_blend_state();
    RPipelineInfo pipelineI{
        .shaderCount = 2,
        .shaders = shaders,
        .vertexAttributeCount = 1,
        .vertexAttributes = &attribute,
        .vertexBindingCount = 1,
        .vertexBindings = &binding,
        .primitiveTopology = RPRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .layout = sEquirectangularPipelineLayout,
        .rasterization = {
            .polygonMode = RPOLYGON_MODE_FILL,
            .cullMode = RCULL_MODE_NONE,
        },
        .depthStencil = {.depthTestEnabled = false},
        .blend = {
            .colorAttachmentCount = 1,
            .colorAttachments = &blendAttachment,
        },
    };
    obj->handle = device.create_pipeline(pipelineI);

    return {obj};
}

void EquirectangularPipeline::destroy(EquirectangularPipeline pipeline)
{
    EquirectangularPipelineObj* obj = pipeline;
    RDevice device = obj->device;

    device.destroy_pipeline(obj->handle);
    device.destroy_shader(obj->vertexShader);
    device.destroy_shader(obj->fragmentShader);

    heap_free(obj);
}

RPipeline EquirectangularPipeline::handle()
{
    return mObj->handle;
}

void equirectangular_cmd_render_to_faces(RDevice device, EquirectangularPipeline pipeline, RImage srcImage, RImage* dstImages, RBuffer* dstBuffers)
{
    RCommandPool cmdPool = device.create_command_pool({RQUEUE_TYPE_GRAPHICS, true});
    RCommandList list = cmdPool.allocate();

    RSetPool setPool = device.create_set_pool({sSingleSampleSetLayout, 1});
    RSet equirectangularImageSet = setPool.allocate();
    RImageLayout layout = RIMAGE_LAYOUT_SHADER_READ_ONLY;
    RSetImageUpdateInfo updateI = RUtil::make_single_set_image_update_info(equirectangularImageSet, 0, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &srcImage);
    device.update_set_images(1, &updateI);

    Vec3 cubePos[36];
    get_cube_mesh_vertex_attributes(cubePos);

    RBuffer cubeVBO = device.create_buffer({RBUFFER_USAGE_VERTEX_BIT, sizeof(cubePos), true});
    cubeVBO.map();
    cubeVBO.map_write(0, sizeof(cubePos), cubePos);
    cubeVBO.unmap();

    RPassColorAttachment faceColorAttachment{};
    faceColorAttachment.colorFormat = dstImages[0].format();
    faceColorAttachment.colorLoadOp = RATTACHMENT_LOAD_OP_DONT_CARE;
    faceColorAttachment.colorStoreOp = RATTACHMENT_STORE_OP_STORE;
    faceColorAttachment.initialLayout = RIMAGE_LAYOUT_UNDEFINED;
    faceColorAttachment.passLayout = RIMAGE_LAYOUT_COLOR_ATTACHMENT;

    RPassInfo facePass{};
    facePass.samples = RSAMPLE_COUNT_1_BIT;
    facePass.colorAttachmentCount = 1;
    facePass.colorAttachments = &faceColorAttachment;
    facePass.colorResolveAttachments = nullptr;
    facePass.depthStencilAttachment = nullptr;
    facePass.dependency = nullptr;

    const Mat4 projMat = Mat4::perspective(LD_PI_2, 1.0f, 0.1f, 10.0f);
    const std::array<Mat4, 6> viewMats{
        Mat4::look_at(Vec3(0.0f), Vec3(+1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)),
        Mat4::look_at(Vec3(0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)),
        Mat4::look_at(Vec3(0.0f), Vec3(0.0f, +1.0f, 0.0f), Vec3(0.0f, 0.0f, +1.0f)),
        Mat4::look_at(Vec3(0.0f), Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f)),
        Mat4::look_at(Vec3(0.0f), Vec3(0.0f, 0.0f, +1.0f), Vec3(0.0f, -1.0f, 0.0f)),
        Mat4::look_at(Vec3(0.0f), Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f)),
    };

    list.begin();

    for (int i = 0; i < 6; i++)
    {
        RImage face = dstImages[i];

        RPassBeginInfo passBI{};
        passBI.width = face.width();
        passBI.height = face.height();
        passBI.pass = facePass;
        passBI.colorAttachmentCount = 1;
        passBI.colorAttachments = &face;
        passBI.depthStencilAttachment = {};
        passBI.clearColors = nullptr;

        Mat4 viewProj = projMat * viewMats[i];
        list.cmd_begin_pass(passBI);
        list.cmd_push_constant(sEquirectangularPipelineLayout, 0, sizeof(Mat4), &viewProj);
        list.cmd_bind_graphics_pipeline(pipeline.handle());
        list.cmd_bind_graphics_sets(sEquirectangularPipelineLayout, 0, 1, &equirectangularImageSet);
        list.cmd_bind_vertex_buffers(0, 1, &cubeVBO);
        list.cmd_draw({.vertexCount = 36, .vertexStart = 0, .instanceCount = 1, .instanceStart = 0});
        list.cmd_end_pass();
    }

    // additional commands to copy image to buffers
    if (dstBuffers)
    {
        for (int i = 0; i < 6; i++)
        {
            RBuffer dstBuffer = dstBuffers[i];
            RImage srcImage = dstImages[i];
            RImageMemoryBarrier barrier = RUtil::make_image_memory_barrier(srcImage, RIMAGE_LAYOUT_COLOR_ATTACHMENT, RIMAGE_LAYOUT_TRANSFER_SRC, RACCESS_COLOR_ATTACHMENT_WRITE_BIT, RACCESS_TRANSFER_READ_BIT);
            list.cmd_image_memory_barrier(RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, RPIPELINE_STAGE_TRANSFER_BIT, barrier);

            RBufferImageCopy region{};
            region.imageWidth = srcImage.width();
            region.imageHeight = srcImage.height();
            region.imageDepth = 1;
            region.imageLayers = 1;
            list.cmd_copy_image_to_buffer(srcImage, RIMAGE_LAYOUT_TRANSFER_SRC, dstBuffer, 1, &region);
        }
    }

    list.end();

    RQueue queue = device.get_graphics_queue();
    RSubmitInfo submitI{};
    submitI.listCount = 1;
    submitI.lists = &list;
    queue.submit(submitI, {});
    queue.wait_idle();

    device.destroy_buffer(cubeVBO);
    device.destroy_set_pool(setPool);
    device.destroy_command_pool(cmdPool);
}

} // namespace LD