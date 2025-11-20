#include <Extra/doctest/doctest.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUtil.h>

#include <array>

#include "RBackendTest.h"

#define IMAGE_WIDTH 256
#define IMAGE_HEIGHT 256

using namespace LD;

static const char sTriangleVSGLSL[] = R"(
#version 460 core

layout (location = 0) out vec3 vColor;

float attr[15] = float[](
    0.0, -0.5,  1.0, 0.0, 0.0,
    +0.5, +0.5, 0.0, 1.0, 0.0,
    -0.5, +0.5, 0.0, 0.0, 1.0
);

void main()
{
    uint base = uint(gl_VertexIndex) * 5;
    vColor = vec3(attr[base + 2], attr[base + 3], attr[base + 4]);
    gl_Position = vec4(attr[base], attr[base + 1], 0.0, 1.0);
}
)";

static const char sQuadVSGLSL[] = R"(
#version 460 core

layout (location = 0) out vec3 vColor;

float attr[30] = float[](
    -0.5, -0.5, 0.0, 0.0, 0.0,
    +0.5, -0.5, 0.0, 1.0, 0.0,
    +0.5, +0.5, 1.0, 1.0, 0.0,
    +0.5, +0.5, 1.0, 1.0, 0.0,
    -0.5, +0.5, 1.0, 0.0, 0.0,
    -0.5, -0.5, 0.0, 0.0, 0.0
);

void main()
{
    uint base = uint(gl_VertexIndex) * 5;
    vColor = vec3(attr[base + 2], attr[base + 3], attr[base + 4]);
    gl_Position = vec4(attr[base], attr[base + 1], 0.0, 1.0);
}
)";

static const char sBasicFSGLSL[] = R"(
#version 460 core

layout (location = 0) in vec3 vColor;
layout (location = 0) out vec4 fColor;

void main()
{
    fColor = vec4(vColor, 1.0);
}
)";

struct RBackendPrimitiveTestInfo
{
    RDeviceBackend backend;
    const char* triangleImageSavePath;
    const char* quadImageSavePath;
};

static void rbackend_primitive_test(const RBackendPrimitiveTestInfo& info)
{
    LD_PROFILE_SCOPE;

    RDeviceInfo deviceI{};
    deviceI.backend = info.backend;
    deviceI.window = nullptr;
    RDevice device = RDevice::create(deviceI);
    CHECK(device);

    RShaderInfo shaderI{};
    shaderI.type = RSHADER_TYPE_VERTEX;
    shaderI.glsl = sTriangleVSGLSL;
    RShader triangleVS = device.create_shader(shaderI);
    CHECK(triangleVS);

    shaderI.type = RSHADER_TYPE_VERTEX;
    shaderI.glsl = sQuadVSGLSL;
    RShader quadVS = device.create_shader(shaderI);
    CHECK(quadVS);

    shaderI.type = RSHADER_TYPE_FRAGMENT;
    shaderI.glsl = sBasicFSGLSL;
    RShader basicFS = device.create_shader(shaderI);
    CHECK(basicFS);

    std::array<RShader, 2> shaders = {triangleVS, basicFS};

    RPipelineBlendState bs = RUtil::make_default_blend_state();
    RPipelineInfo pipelineI{};
    pipelineI.shaderCount = (uint32_t)shaders.size();
    pipelineI.shaders = shaders.data();
    pipelineI.vertexAttributeCount = 0;
    pipelineI.vertexBindingCount = 0;
    pipelineI.primitiveTopology = RPRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineI.layout.setLayoutCount = 0;
    pipelineI.blend.colorAttachmentCount = 1;
    pipelineI.blend.colorAttachments = &bs;
    RPipeline trianglePipeline = device.create_pipeline(pipelineI);
    CHECK(trianglePipeline);

    shaders[0] = quadVS;
    RPipeline quadPipeline = device.create_pipeline(pipelineI);
    CHECK(quadPipeline);

    RBufferInfo bufferI{};
    bufferI.usage = RBUFFER_USAGE_TRANSFER_DST_BIT;
    bufferI.size = IMAGE_WIDTH * IMAGE_HEIGHT * 4;
    bufferI.hostVisible = true;
    RBuffer hostBuffer = device.create_buffer(bufferI);
    CHECK(hostBuffer);

    RImageInfo imageI{};
    imageI.usage = RIMAGE_USAGE_COLOR_ATTACHMENT_BIT | RIMAGE_USAGE_TRANSFER_SRC_BIT;
    imageI.type = RIMAGE_TYPE_2D;
    imageI.samples = RSAMPLE_COUNT_1_BIT;
    imageI.format = RFORMAT_RGBA8;
    imageI.layers = 1;
    imageI.width = IMAGE_WIDTH;
    imageI.height = IMAGE_HEIGHT;
    imageI.depth = 1;
    RImage colorImage = device.create_image(imageI);
    CHECK(colorImage);

    RCommandPoolInfo cmdPoolI{};
    cmdPoolI.queueType = RQUEUE_TYPE_GRAPHICS;
    cmdPoolI.hintTransient = true;
    cmdPoolI.listResettable = true;
    RCommandPool cmdPool = device.create_command_pool(cmdPoolI);
    CHECK(cmdPool);

    RCommandList cmdList = cmdPool.allocate();
    CHECK(cmdList);

    RPassColorAttachment passCA{};
    passCA.colorFormat = RFORMAT_RGBA8;
    passCA.colorLoadOp = RATTACHMENT_LOAD_OP_CLEAR;
    passCA.colorStoreOp = RATTACHMENT_STORE_OP_STORE;
    passCA.initialLayout = RIMAGE_LAYOUT_UNDEFINED;
    passCA.passLayout = RIMAGE_LAYOUT_COLOR_ATTACHMENT;
    RClearColorValue clear = RUtil::make_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    RPassBeginInfo passBI{};
    passBI.width = IMAGE_WIDTH;
    passBI.height = IMAGE_HEIGHT;
    passBI.colorAttachmentCount = 1;
    passBI.colorAttachments = &colorImage;
    passBI.clearColors = &clear;
    passBI.pass.samples = RSAMPLE_COUNT_1_BIT;
    passBI.pass.colorAttachmentCount = 1;
    passBI.pass.colorAttachments = &passCA;
    passBI.pass.dependency = nullptr;
    cmdList.begin();
    cmdList.cmd_begin_pass(passBI);
    cmdList.cmd_bind_graphics_pipeline(trianglePipeline);
    cmdList.cmd_draw({3, 1, 0, 0});
    cmdList.cmd_end_pass();
    RImageMemoryBarrier barrier{}; // copy rendered contents to host visible buffer
    barrier.image = colorImage;
    barrier.oldLayout = RIMAGE_LAYOUT_COLOR_ATTACHMENT;
    barrier.newLayout = RIMAGE_LAYOUT_TRANSFER_SRC;
    barrier.srcAccess = RACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccess = RACCESS_TRANSFER_READ_BIT;
    cmdList.cmd_image_memory_barrier(RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, RPIPELINE_STAGE_TRANSFER_BIT, barrier);
    RBufferImageCopy region{};
    region.imageWidth = IMAGE_WIDTH;
    region.imageHeight = IMAGE_HEIGHT;
    region.imageDepth = 1;
    region.imageLayers = 1;
    cmdList.cmd_copy_image_to_buffer(colorImage, RIMAGE_LAYOUT_TRANSFER_SRC, hostBuffer, 1, &region);
    cmdList.end();

    RQueue queue = device.get_graphics_queue();
    RSubmitInfo submitI{};
    submitI.listCount = 1;
    submitI.lists = &cmdList;
    queue.submit(submitI, {});
    device.wait_idle();

    hostBuffer.map();
    void* pixels = hostBuffer.map_read(0, hostBuffer.size());
    BitmapView view{IMAGE_WIDTH, IMAGE_HEIGHT, BITMAP_CHANNEL_RGBA, (const char*)pixels};
    Bitmap::save_to_disk(view, info.triangleImageSavePath);
    hostBuffer.unmap();

    cmdList.reset();
    cmdList.begin();
    cmdList.cmd_begin_pass(passBI);
    cmdList.cmd_bind_graphics_pipeline(quadPipeline);
    cmdList.cmd_draw({6, 1, 0, 0});
    cmdList.cmd_end_pass();
    cmdList.cmd_image_memory_barrier(RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, RPIPELINE_STAGE_TRANSFER_BIT, barrier);
    cmdList.cmd_copy_image_to_buffer(colorImage, RIMAGE_LAYOUT_TRANSFER_SRC, hostBuffer, 1, &region);
    cmdList.end();

    queue.submit(submitI, {});
    device.wait_idle();

    hostBuffer.map();
    pixels = hostBuffer.map_read(0, hostBuffer.size());
    BitmapView view2 = {IMAGE_WIDTH, IMAGE_HEIGHT, BITMAP_CHANNEL_RGBA, (const char*)pixels};
    Bitmap::save_to_disk(view2, info.quadImageSavePath);
    hostBuffer.unmap();

    device.destroy_command_pool(cmdPool);
    device.destroy_image(colorImage);
    device.destroy_buffer(hostBuffer);
    device.destroy_pipeline(quadPipeline);
    device.destroy_pipeline(trianglePipeline);
    device.destroy_shader(basicFS);
    device.destroy_shader(quadVS);
    device.destroy_shader(triangleVS);
    RDevice::destroy(device);
}

TEST_CASE("RBackendPrimitiveTest")
{
    RBackendPrimitiveTestInfo info{};
    info.backend = RDEVICE_BACKEND_VULKAN;
    info.triangleImageSavePath = "./vk_triangle.png";
    info.quadImageSavePath = "./vk_quad.png";
    rbackend_primitive_test(info);

    info.backend = RDEVICE_BACKEND_OPENGL;
    info.triangleImageSavePath = "./gl_triangle.png";
    info.quadImageSavePath = "./gl_quad.png";
    rbackend_primitive_test(info);

    // TODO: probably want a golden image as ground truth,
    //       even if both backends generate identical output,
    //       they could still be both wrong.
    double mse;
    CHECK(compute_bitmap_mse("./vk_triangle.png", "./gl_triangle.png", mse));
    CHECK(mse < BITMAP_MSE_TOLERANCE);

    CHECK(compute_bitmap_mse("./vk_quad.png", "./gl_quad.png", mse));
    CHECK(mse < BITMAP_MSE_TOLERANCE);
}
