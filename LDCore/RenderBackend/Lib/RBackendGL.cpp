#include <Ludens/Header/Assert.h>

// OpenGL 4.6 Core Profile
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "RBackendObj.h"
#include "RShaderCompiler.h"
#include "RUtilCommon.h"
#include "RUtilGL.h"

namespace LD {

struct RDeviceGLObj;

static void gl_buffer_map(RBufferObj* self);
static void* gl_buffer_map_read(RBufferObj* self, uint64_t offset, uint64_t size);
static void gl_buffer_map_write(RBufferObj* self, uint64_t offset, uint64_t size, const void* data);
static void gl_buffer_unmap(RBufferObj* self);

static constexpr RBufferAPI sRBufferGLAPI = {
    .map = &gl_buffer_map,
    .map_read = &gl_buffer_map_read,
    .map_write = &gl_buffer_map_write,
    .unmap = &gl_buffer_unmap,
};

/// @brief OpenGL buffer object.
struct RBufferGLObj : RBufferObj
{
    RBufferGLObj()
    {
        api = &sRBufferGLAPI;
    }

    struct
    {
        GLuint handle;
    } gl;
};

/// @brief OpenGL image object.
struct RImageGLObj : RImageObj
{
    struct
    {
        GLenum target;
        GLenum internalFormat;
        GLenum dataFormat;
        GLenum dataType;
        GLuint handle;
    } gl;
};

/// @brief OpenGL render pass object.
struct RPassGLObj : RPassObj
{
};

/// @brief OpenGL framebuffer object.
struct RFramebufferGLObj : RFramebufferObj
{
    struct
    {
        GLuint handle;
    } gl;
};

static RCommandList gl_command_pool_allocate(RCommandPoolObj* self, RCommandListObj* listObj);
static void gl_command_pool_reset(RCommandPoolObj* self);

static constexpr RCommandPoolAPI sRCommandPoolGLAPI = {
    .allocate = &gl_command_pool_allocate,
    .reset = &gl_command_pool_reset,
};

/// @brief OpenGL command pool object.
struct RCommandPoolGLObj : RCommandPoolObj
{
    RCommandPoolGLObj()
    {
        api = &sRCommandPoolGLAPI;
    }
};

/// @brief OpenGL command list object.
struct RCommandListGLObj : RCommandListObj
{
    RCommandListGLObj()
    {
        // NOTE: OpenGL does not implement the API for command recording,
        //       it relies on the base class capturing all commands for
        //       deferred execution (we have to respect submission order between lists).
        api = nullptr;

        LinearAllocatorInfo laI{};
        laI.usage = MEMORY_USAGE_RENDER;
        laI.capacity = 2048;
        laI.isMultiPage = true;
        captureLA = LinearAllocator::create(laI);
    }

    ~RCommandListGLObj()
    {
        if (captureLA)
        {
            LinearAllocator::destroy(captureLA);
            captureLA = {};
        }
    }
};

/// @brief OpenGL shader object.
struct RShaderGLObj : RShaderObj
{
    struct
    {
        GLuint handle;
    } gl;
};

/// @brief OpenGL set layout object.
struct RSetLayoutGLObj : RShaderObj
{
    std::vector<RSetBindingInfo> bindings;
};

/// @brief OpenGL pipeline layout object.
struct RPipelineLayoutGLObj : RPipelineLayoutObj
{
};

static void gl_pipeline_create_variant(RPipelineObj* baseObj)
{
}

static constexpr RPipelineAPI sRPipelineGLAPI = {
    .create_variant = &gl_pipeline_create_variant};

/// @brief OpenGL pipeline object.
struct RPipelineGLObj : RPipelineObj
{
    RPipelineGLObj()
    {
        api = &sRPipelineGLAPI;
    }

    struct
    {
        GLenum primitiveMode;
        GLuint handle;
    } gl;
};

static void gl_queue_wait_idle(RQueueObj* baseSelf);
static void gl_queue_submit(RQueueObj* baseSelf, const RSubmitInfo& submitI, RFence fence);

static constexpr RQueueAPI sRQueueGLAPI = {
    .wait_idle = &gl_queue_wait_idle,
    .submit = &gl_queue_submit,
};

struct RQueueGLObj : RQueueObj
{
    RQueueGLObj()
    {
        api = &sRQueueGLAPI;
    }

    struct
    {
        RDeviceGLObj* deviceObj;
    } gl;
};

static size_t gl_device_get_obj_size(RType objType);

static void gl_device_buffer_ctor(RBufferObj* baseObj);
static void gl_device_buffer_dtor(RBufferObj* baseObj);
static RBuffer gl_device_create_buffer(RDeviceObj* baseSelf, const RBufferInfo& bufferI, RBufferObj* baseObj);
static void gl_device_destroy_buffer(RDeviceObj* baseSelf, RBuffer buffer);

static void gl_device_image_ctor(RImageObj* baseObj);
static void gl_device_image_dtor(RImageObj* baseObj);
static RImage gl_device_create_image(RDeviceObj* baseSelf, const RImageInfo& imageI, RImageObj* baseObj);
static void gl_device_destroy_image(RDeviceObj* baseSelf, RImage image);

static void gl_device_pass_ctor(RPassObj* baseObj);
static void gl_device_pass_dtor(RPassObj* baseObj);
static void gl_device_create_pass(RDeviceObj* baseSelf, const RPassInfo& passI, RPassObj* baseObj);
static void gl_device_destroy_pass(RDeviceObj* baseSelf, RPassObj* baseObj);

static void gl_device_framebuffer_ctor(RFramebufferObj* baseObj);
static void gl_device_framebuffer_dtor(RFramebufferObj* baseObj);
static void gl_device_create_framebuffer(RDeviceObj* baseSelf, const RFramebufferInfo& fbI, RFramebufferObj* baseObj);
static void gl_device_destroy_framebuffer(RDeviceObj* baseSelf, RFramebufferObj* baseObj);

static void gl_device_command_pool_ctor(RCommandPoolObj* baseObj);
static void gl_device_command_pool_dtor(RCommandPoolObj* baseObj);
static RCommandPool gl_device_create_command_pool(RDeviceObj* baseSelf, const RCommandPoolInfo& poolI, RCommandPoolObj* baseObj);
static void gl_device_destroy_command_pool(RDeviceObj* baseSelf, RCommandPool pool);

static void gl_device_command_list_ctor(RCommandListObj* baseObj);
static void gl_device_command_list_dtor(RCommandListObj* baseObj);

static void gl_device_shader_ctor(RShaderObj* baseObj);
static void gl_device_shader_dtor(RShaderObj* baseObj);
static RShader gl_device_create_shader(RDeviceObj* baseSelf, const RShaderInfo& shaderI, RShaderObj* baseObj);
static void gl_device_destroy_shader(RDeviceObj* baseSelf, RShader shader);

static void gl_device_set_layout_ctor(RSetLayoutObj* baseObj);
static void gl_device_set_layout_dtor(RSetLayoutObj* baseObj);
static void gl_device_create_set_layout(RDeviceObj* baseSelf, const RSetLayoutInfo& setLI, RSetLayoutObj* baseObj);
static void gl_device_destroy_set_layout(RDeviceObj* baseSelf, RSetLayoutObj* baseObj);

static void gl_device_pipeline_layout_ctor(RPipelineLayoutObj* baseObj);
static void gl_device_pipeline_layout_dtor(RPipelineLayoutObj* baseObj);
static void gl_device_create_pipeline_layout(RDeviceObj* baseSelf, const RPipelineLayoutInfo& shaderI, RPipelineLayoutObj* baseObj);
static void gl_device_destroy_pipeline_layout(RDeviceObj* baseSelf, RPipelineLayoutObj* baseObj);

static void gl_device_pipeline_ctor(RPipelineObj* baseObj);
static void gl_device_pipeline_dtor(RPipelineObj* baseObj);
static RPipeline gl_device_create_pipeline(RDeviceObj* baseSelf, const RPipelineInfo& pipelineI, RPipelineObj* baseObj);
static void gl_device_destroy_pipeline(RDeviceObj* baseSelf, RPipeline pipeline);
static void gl_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI);
static RQueue gl_device_get_graphics_queue(RDeviceObj* self);
static void gl_device_wait_idle(RDeviceObj* self);

static constexpr RDeviceAPI sRDeviceGLAPI = {
    .get_obj_size = &gl_device_get_obj_size,
    .semaphore_ctor = nullptr,
    .semaphore_dtor = nullptr,
    .create_semaphore = nullptr,
    .destroy_semaphore = nullptr,
    .fence_ctor = nullptr,
    .fence_dtor = nullptr,
    .create_fence = nullptr,
    .destroy_fence = nullptr,
    .buffer_ctor = &gl_device_buffer_ctor,
    .buffer_dtor = &gl_device_buffer_dtor,
    .create_buffer = &gl_device_create_buffer,
    .destroy_buffer = &gl_device_destroy_buffer,
    .image_ctor = &gl_device_image_ctor,
    .image_dtor = &gl_device_image_dtor,
    .create_image = &gl_device_create_image,
    .destroy_image = &gl_device_destroy_image,
    .pass_ctor = &gl_device_pass_ctor,
    .pass_dtor = &gl_device_pass_dtor,
    .create_pass = &gl_device_create_pass,
    .destroy_pass = &gl_device_destroy_pass,
    .framebuffer_ctor = &gl_device_framebuffer_ctor,
    .framebuffer_dtor = &gl_device_framebuffer_dtor,
    .create_framebuffer = &gl_device_create_framebuffer,
    .destroy_framebuffer = &gl_device_destroy_framebuffer,
    .command_pool_ctor = &gl_device_command_pool_ctor,
    .command_pool_dtor = &gl_device_command_pool_dtor,
    .create_command_pool = &gl_device_create_command_pool,
    .destroy_command_pool = &gl_device_destroy_command_pool,
    .command_list_ctor = &gl_device_command_list_ctor,
    .command_list_dtor = &gl_device_command_list_dtor,
    .shader_ctor = &gl_device_shader_ctor,
    .shader_dtor = &gl_device_shader_dtor,
    .create_shader = &gl_device_create_shader,
    .destroy_shader = &gl_device_destroy_shader,
    .set_pool_ctor = nullptr,
    .set_pool_dtor = nullptr,
    .create_set_pool = nullptr,
    .destroy_set_pool = nullptr,
    .set_layout_ctor = &gl_device_set_layout_ctor,
    .set_layout_dtor = &gl_device_set_layout_dtor,
    .create_set_layout = &gl_device_create_set_layout,
    .destroy_set_layout = &gl_device_destroy_set_layout,
    .pipeline_layout_ctor = &gl_device_pipeline_layout_ctor,
    .pipeline_layout_dtor = &gl_device_pipeline_layout_dtor,
    .create_pipeline_layout = &gl_device_create_pipeline_layout,
    .destroy_pipeline_layout = &gl_device_destroy_pipeline_layout,
    .pipeline_ctor = &gl_device_pipeline_ctor,
    .pipeline_dtor = &gl_device_pipeline_dtor,
    .create_pipeline = &gl_device_create_pipeline,
    .create_compute_pipeline = nullptr,
    .destroy_pipeline = &gl_device_destroy_pipeline,
    .pipeline_variant_pass = &gl_device_pipeline_variant_pass,
    .pipeline_variant_color_write_mask = nullptr,
    .pipeline_variant_depth_test_enable = nullptr,
    .update_set_images = nullptr,
    .update_set_buffers = nullptr,
    .next_frame = nullptr,
    .present_frame = nullptr,
    .get_depth_stencil_formats = nullptr,
    .get_max_sample_count = nullptr,
    .get_swapchain_color_format = nullptr,
    .get_swapchain_color_attachment = nullptr,
    .get_swapchain_image_count = nullptr,
    .get_swapchain_extent = nullptr,
    .get_frames_in_flight_count = nullptr,
    .get_graphics_queue = &gl_device_get_graphics_queue,
    .wait_idle = &gl_device_wait_idle,
};

/// @brief OpenGL render device object.
struct RDeviceGLObj : RDeviceObj
{
    RDeviceGLObj()
    {
        backend = RDEVICE_BACKEND_OPENGL;
        api = &sRDeviceGLAPI;

        queueObj.api = &sRQueueGLAPI;
        queueObj.gl.deviceObj = this;
    }

    RQueueGLObj queueObj;
    RPipelineGLObj* boundGraphicsPipeline = nullptr;
};

static void gl_copy_image_to_buffer(RImageGLObj* imageObj, RBufferGLObj* bufferObj, const RBufferImageCopy& region);

static void gl_command_execute(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_execute_begin_pass(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_bind_graphics_pipeline(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_draw(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_draw_indexed(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_end_pass(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_image_memory_barrier(const RCommandType* type, RDeviceGLObj* deviceObj);
static void gl_command_copy_image_to_buffer(const RCommandType* type, RDeviceGLObj* deviceObj);

static constexpr void (*sCommandTable[])(const RCommandType*, RDeviceGLObj*) = {
    &gl_command_execute_begin_pass,
    nullptr,
    &gl_command_bind_graphics_pipeline,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &gl_command_draw,
    &gl_command_draw_indexed,
    &gl_command_end_pass,
    nullptr,
    nullptr,
    &gl_command_image_memory_barrier,
    nullptr,
    nullptr,
    &gl_command_copy_image_to_buffer,
    nullptr,
};

static_assert(sizeof(sCommandTable) / sizeof(*sCommandTable) == (size_t)RCOMMAND_TYPE_ENUM_COUNT);

// clang-format off
struct RTypeGL
{
    RType type;
    size_t byteSize;
} sTypeGLTable[] = {
    { RTYPE_DEVICE,          sizeof(RDeviceGLObj) },
    { RTYPE_SEMAPHORE,       0},
    { RTYPE_FENCE,           0},
    { RTYPE_BUFFER,          sizeof(RBufferGLObj) },
    { RTYPE_IMAGE,           sizeof(RImageGLObj) },
    { RTYPE_SHADER,          sizeof(RShaderGLObj) },
    { RTYPE_SET_LAYOUT,      sizeof(RSetLayoutGLObj) },
    { RTYPE_SET,             0},
    { RTYPE_SET_POOL,        0},
    { RTYPE_PASS,            sizeof(RPassGLObj) },
    { RTYPE_FRAMEBUFFER,     sizeof(RFramebufferGLObj) },
    { RTYPE_PIPELINE_LAYOUT, sizeof(RPipelineLayoutGLObj) },
    { RTYPE_PIPELINE,        sizeof(RPipelineGLObj) },
    { RTYPE_COMMAND_LIST,    sizeof(RCommandListGLObj) },
    { RTYPE_COMMAND_POOL,    sizeof(RCommandPoolGLObj) },
    { RTYPE_QUEUE,           sizeof(RQueueGLObj) },
};
// clang-format on

static void gl_buffer_map(RBufferObj* baseSelf)
{
    auto* self = (RBufferGLObj*)baseSelf;

    self->hostMap = glMapNamedBuffer(self->gl.handle, GL_READ_WRITE);
}

static void* gl_buffer_map_read(RBufferObj* baseSelf, uint64_t offset, uint64_t size)
{
    auto* self = (RBufferGLObj*)baseSelf;
    char* src = (char*)self->hostMap + offset;

    return (void*)src;
}

static void gl_buffer_map_write(RBufferObj* baseSelf, uint64_t offset, uint64_t size, const void* data)
{
    auto* self = (RBufferGLObj*)baseSelf;
    char* dst = (char*)self->hostMap + offset;

    memcpy(dst, data, size);
}

static void gl_buffer_unmap(RBufferObj* baseSelf)
{
    auto* self = (RBufferGLObj*)baseSelf;

    glUnmapNamedBuffer(self->gl.handle);

    self->hostMap = nullptr;
}

static RCommandList gl_command_pool_allocate(RCommandPoolObj* self, RCommandListObj* listObj)
{
    // TODO:
    auto* obj = (RCommandListGLObj*)listObj;

    return RCommandList(obj);
}

static void gl_command_pool_reset(RCommandPoolObj* self)
{
    // TODO:
}

static void gl_queue_wait_idle(RQueueObj* baseSelf)
{
    // TODO: flush all submissions

    (void)baseSelf;
}

static void gl_queue_submit(RQueueObj* baseSelf, const RSubmitInfo& submitI, RFence fence)
{
    auto* self = (RQueueGLObj*)baseSelf;

    // TODO: simulate semaphore synchronization
    LD_ASSERT(submitI.waitCount == 0);
    LD_ASSERT(submitI.signalCount == 0);

    // execute commands in submission order
    for (uint32_t i = 0; i < submitI.listCount; i++)
    {
        RCommandListGLObj* listObj = (RCommandListGLObj*)submitI.lists[i].unwrap();

        for (const RCommandType* cmd : listObj->captures)
        {
            gl_command_execute(cmd, self->gl.deviceObj);
        }
    }
}

size_t gl_device_byte_size()
{
    return sizeof(RDeviceGLObj);
}

void gl_device_ctor(RDeviceObj* baseObj)
{
    auto* obj = (RDeviceGLObj*)baseObj;

    new (obj) RDeviceGLObj();
}

void gl_device_dtor(RDeviceObj* baseObj)
{
    auto* obj = (RDeviceGLObj*)baseObj;

    obj->~RDeviceGLObj();
}

void gl_create_device(struct RDeviceObj* baseObj, const RDeviceInfo& info)
{
    auto* obj = (RDeviceGLObj*)baseObj;

    if (obj->isHeadless && !obj->glfw)
    {
        // we still need an OpenGL context, create an invisible window for headless mode.
        int result = glfwInit();
        LD_ASSERT(result == GLFW_TRUE);

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        obj->glfw = glfwCreateWindow(1, 1, "headless", nullptr, nullptr);
        glfwMakeContextCurrent(obj->glfw);
    }

    // NOTE: glfwMakeContextCurrent() should already be called
    //       so that there is a valid OpenGL context on main thread.
    int ret = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    LD_ASSERT(ret != 0);
}

void gl_destroy_device(struct RDeviceObj* baseObj)
{
    auto* obj = (RDeviceGLObj*)baseObj;

    if (obj->isHeadless && obj->glfw)
    {
        glfwDestroyWindow(obj->glfw);
        glfwTerminate();
    }
}

static size_t gl_device_get_obj_size(RType objType)
{
    return sTypeGLTable[(int)objType].byteSize;
}

static void gl_device_buffer_ctor(RBufferObj* baseObj)
{
    auto* obj = (RBufferGLObj*)baseObj;

    new (obj) RBufferGLObj();
}

static void gl_device_buffer_dtor(RBufferObj* baseObj)
{
    auto* obj = (RBufferGLObj*)baseObj;

    obj->~RBufferGLObj();
}

static RBuffer gl_device_create_buffer(RDeviceObj* baseSelf, const RBufferInfo& bufferI, RBufferObj* baseObj)
{
    auto* obj = (RBufferGLObj*)baseObj;

    // TODO:
    glCreateBuffers(1, &obj->gl.handle);
    glNamedBufferStorage(obj->gl.handle, bufferI.size, nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    GLenum error = glGetError();

    return RBuffer(obj);
}

static void gl_device_destroy_buffer(RDeviceObj* baseSelf, RBuffer buffer)
{
    auto* obj = (RBufferGLObj*)buffer.unwrap();

    // TODO:
    glDeleteBuffers(1, &obj->gl.handle);
}

static void gl_device_image_ctor(RImageObj* baseObj)
{
    auto* obj = (RImageGLObj*)baseObj;

    new (obj) RImageGLObj();
}

static void gl_device_image_dtor(RImageObj* baseObj)
{
    auto* obj = (RImageGLObj*)baseObj;

    obj->~RImageGLObj();
}

static RImage gl_device_create_image(RDeviceObj* baseSelf, const RImageInfo& imageI, RImageObj* baseObj)
{
    auto* obj = (RImageGLObj*)baseObj;

    RUtil::cast_format_gl(imageI.format, obj->gl.internalFormat, obj->gl.dataFormat, obj->gl.dataType);
    RUtil::cast_image_type_gl(imageI.type, obj->gl.target);

    glCreateTextures(obj->gl.target, 1, &obj->gl.handle);

    GLsizei width = (GLsizei)obj->info.width;
    GLsizei height = (GLsizei)obj->info.height;

    switch (obj->gl.target)
    {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_CUBE_MAP:
        glTextureStorage2D(obj->gl.handle, 1, obj->gl.internalFormat, width, height);
        break;
    default:
        LD_UNREACHABLE;
    }

    if (imageI.usage & RIMAGE_USAGE_SAMPLED_BIT)
    {
        GLenum addressMode;
        RUtil::cast_sampler_address_mode_gl(imageI.sampler.addressMode, addressMode);
        glTextureParameteri(obj->gl.target, GL_TEXTURE_WRAP_S, addressMode);
        glTextureParameteri(obj->gl.target, GL_TEXTURE_WRAP_T, addressMode);

        GLenum minFilter, magFilter;
        RUtil::cast_filter_gl(imageI.sampler, minFilter, magFilter);
        glTextureParameteri(obj->gl.target, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(obj->gl.target, GL_TEXTURE_MAG_FILTER, magFilter);
    }

    return RImage(obj);
}

static void gl_device_destroy_image(RDeviceObj* baseSelf, RImage image)
{
    auto* obj = (RImageGLObj*)image.unwrap();

    glDeleteTextures(1, &obj->gl.handle);
}

static void gl_device_pass_ctor(RPassObj* baseObj)
{
    auto* obj = (RPassGLObj*)baseObj;

    new (obj) RPassGLObj();
}

static void gl_device_pass_dtor(RPassObj* baseObj)
{
    auto* obj = (RPassGLObj*)baseObj;

    obj->~RPassGLObj();
}

static void gl_device_create_pass(RDeviceObj* baseSelf, const RPassInfo& passI, RPassObj* baseObj)
{
    (void*)baseObj;
}

static void gl_device_destroy_pass(RDeviceObj* baseSelf, RPassObj* baseObj)
{
    (void*)baseObj;
}

static void gl_device_framebuffer_ctor(RFramebufferObj* baseObj)
{
    auto* obj = (RFramebufferGLObj*)baseObj;

    new (obj) RFramebufferGLObj();
}

static void gl_device_framebuffer_dtor(RFramebufferObj* baseObj)
{
    auto* obj = (RFramebufferGLObj*)baseObj;

    obj->~RFramebufferGLObj();
}

static void gl_device_create_framebuffer(RDeviceObj* baseSelf, const RFramebufferInfo& fbI, RFramebufferObj* baseObj)
{
    auto* obj = (RFramebufferGLObj*)baseObj;

    glCreateFramebuffers(1, &obj->gl.handle);
    glBindFramebuffer(GL_FRAMEBUFFER, obj->gl.handle);

    for (uint32_t i = 0; i < fbI.colorAttachmentCount; i++)
    {
        auto* imageObj = (RImageGLObj*)fbI.colorAttachments[i].unwrap();
        LD_ASSERT(imageObj->info.type == RIMAGE_TYPE_2D);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, imageObj->gl.handle, 0);
    }

    // TODO: GL_DEPTH, GL_STENCIL, GL_DEPTH_STENCIL attachments
    LD_ASSERT(!fbI.depthStencilAttachment);
    LD_ASSERT(!fbI.colorResolveAttachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LD_UNREACHABLE;
}

static void gl_device_destroy_framebuffer(RDeviceObj* baseSelf, RFramebufferObj* baseObj)
{
    auto* obj = (RFramebufferGLObj*)baseObj;

    glDeleteFramebuffers(1, &obj->gl.handle);
}

static void gl_device_command_pool_ctor(RCommandPoolObj* baseObj)
{
    auto* obj = (RCommandPoolGLObj*)baseObj;

    new (obj) RCommandPoolGLObj();
}

static void gl_device_command_pool_dtor(RCommandPoolObj* baseObj)
{
    auto* obj = (RCommandPoolGLObj*)baseObj;

    obj->~RCommandPoolGLObj();
}

static RCommandPool gl_device_create_command_pool(RDeviceObj* baseSelf, const RCommandPoolInfo& poolI, RCommandPoolObj* baseObj)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RCommandPoolGLObj*)baseObj;

    // TODO:
    return RCommandPool(obj);
}

static void gl_device_destroy_command_pool(RDeviceObj* baseSelf, RCommandPool pool)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RCommandPoolGLObj*)pool.unwrap();

    // TODO:
}

static void gl_device_command_list_ctor(RCommandListObj* baseObj)
{
    auto* obj = (RCommandListGLObj*)baseObj;

    new (obj) RCommandListGLObj();
}

static void gl_device_command_list_dtor(RCommandListObj* baseObj)
{
    auto* obj = (RCommandListGLObj*)baseObj;

    obj->~RCommandListGLObj();
}

static void gl_device_shader_ctor(RShaderObj* baseObj)
{
    auto* obj = (RShaderGLObj*)baseObj;

    new (obj) RShaderGLObj();
}

static void gl_device_shader_dtor(RShaderObj* baseObj)
{
    auto* obj = (RShaderGLObj*)baseObj;

    obj->~RShaderGLObj();
}

static RShader gl_device_create_shader(RDeviceObj* baseSelf, const RShaderInfo& shaderI, RShaderObj* baseObj)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RShaderGLObj*)baseObj;

    GLenum shaderType;
    RUtil::cast_shader_type_gl(shaderI.type, shaderType);
    obj->gl.handle = glCreateShader(shaderType);

    // Vulkan-GLSL -> SPIRV -> OpenGL-GLSL
    RShaderCompiler compiler;
    std::string glsl;
    bool success = compiler.compile_to_opengl_glsl(shaderI.type, shaderI.glsl, glsl);

    if (!success)
        return {};

    const GLchar* source = (const GLchar*)glsl.c_str();
    GLint len = (GLint)strlen(source);
    glShaderSource(obj->gl.handle, 1, &source, &len);
    glCompileShader(obj->gl.handle);

    GLint status;
    glGetShaderiv(obj->gl.handle, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buf[512];
        GLsizei length;
        glGetShaderInfoLog(obj->gl.handle, sizeof(buf), &length, buf);
        std::cout << buf << std::endl;
        LD_UNREACHABLE;
    }

    return RShader(obj);
}

static void gl_device_destroy_shader(RDeviceObj* baseSelf, RShader shader)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RShaderGLObj*)shader.unwrap();

    glDeleteShader(obj->gl.handle);
}

static void gl_device_set_layout_ctor(RSetLayoutObj* baseObj)
{
    auto* obj = (RSetLayoutGLObj*)baseObj;

    new (obj) RSetLayoutGLObj();
}

static void gl_device_set_layout_dtor(RSetLayoutObj* baseObj)
{
    auto* obj = (RSetLayoutGLObj*)baseObj;

    obj->~RSetLayoutGLObj();
}

static void gl_device_create_set_layout(RDeviceObj* baseSelf, const RSetLayoutInfo& setLI, RSetLayoutObj* baseObj)
{
    auto* obj = (RSetLayoutGLObj*)baseObj;

    obj->bindings.resize((size_t)setLI.bindingCount);
    for (size_t i = 0; i < setLI.bindingCount; i++)
        obj->bindings[i] = setLI.bindings[i];
}

static void gl_device_destroy_set_layout(RDeviceObj* baseSelf, RSetLayoutObj* baseObj)
{
    auto* obj = (RSetLayoutGLObj*)baseObj;

    obj->bindings.clear();
}

static void gl_device_pipeline_layout_ctor(RPipelineLayoutObj* baseObj)
{
    auto* obj = (RPipelineLayoutGLObj*)baseObj;

    new (obj) RPipelineLayoutGLObj();
}

static void gl_device_pipeline_layout_dtor(RPipelineLayoutObj* baseObj)
{
    auto* obj = (RPipelineLayoutGLObj*)baseObj;

    obj->~RPipelineLayoutGLObj();
}

static void gl_device_create_pipeline_layout(RDeviceObj* baseSelf, const RPipelineLayoutInfo& shaderI, RPipelineLayoutObj* baseObj)
{
    auto* obj = (RPipelineLayoutGLObj*)baseObj;

    // TODO:
}

static void gl_device_destroy_pipeline_layout(RDeviceObj* baseSelf, RPipelineLayoutObj* baseObj)
{
    auto* obj = (RPipelineLayoutGLObj*)baseObj;

    // TODO:
}

static void gl_device_pipeline_ctor(RPipelineObj* baseObj)
{
    auto* obj = (RPipelineGLObj*)baseObj;

    new (obj) RPipelineGLObj();
}

static void gl_device_pipeline_dtor(RPipelineObj* baseObj)
{
    auto* obj = (RPipelineGLObj*)baseObj;

    obj->~RPipelineGLObj();
}

static RPipeline gl_device_create_pipeline(RDeviceObj* baseSelf, const RPipelineInfo& pipelineI, RPipelineObj* baseObj)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RPipelineGLObj*)baseObj;

    obj->gl.handle = glCreateProgram();

    for (uint32_t i = 0; i < pipelineI.shaderCount; i++)
    {
        RShaderGLObj* shaderObj = (RShaderGLObj*)pipelineI.shaders[i].unwrap();
        glAttachShader(obj->gl.handle, shaderObj->gl.handle);
    }

    glLinkProgram(obj->gl.handle);

    GLint status;
    glGetProgramiv(obj->gl.handle, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buf[512];
        glGetProgramInfoLog(obj->gl.handle, sizeof(buf), nullptr, buf);
        std::cout << buf << std::endl;
        LD_UNREACHABLE;
    }

    RUtil::cast_primitive_topology_gl(pipelineI.primitiveTopology, obj->gl.primitiveMode);

    return RPipeline(obj);
}

static void gl_device_destroy_pipeline(RDeviceObj* baseSelf, RPipeline pipeline)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RPipelineGLObj*)pipeline.unwrap();

    glDeleteProgram(obj->gl.handle);
}

static void gl_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI)
{
    (void)self;
    (void)pipelineObj;
}

static RQueue gl_device_get_graphics_queue(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceGLObj*)baseSelf;

    return RQueue(&self->queueObj);
}

static void gl_device_wait_idle(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceGLObj*)baseSelf;

    gl_queue_wait_idle(&self->queueObj);
}

static void gl_command_execute(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(sCommandTable[(int)*type]);

    sCommandTable[(int)*type](type, deviceObj);
}

void gl_command_execute_begin_pass(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(*type == RCOMMAND_BEGIN_PASS);

    const auto& cmd = *(const RCommandBeginPass*)type;
    auto* fbObj = (RFramebufferGLObj*)cmd.framebufferObj;

    glViewport(0, 0, (GLsizei)cmd.width, (GLsizei)cmd.height);
    glBindFramebuffer(GL_FRAMEBUFFER, fbObj->gl.handle);

    // clear color attachments
    const uint32_t colorAttachmentCount = cmd.pass.colorAttachmentCount;
    for (uint32_t i = 0; i < colorAttachmentCount; i++)
    {
        const RPassColorAttachment& passColorAttachment = cmd.pass.colorAttachments[i];

        if (passColorAttachment.colorLoadOp == RATTACHMENT_LOAD_OP_CLEAR)
        {
            // TODO: this assumes floating point color attachment format
            glClearBufferfv(GL_COLOR, (GLint)i, (const GLfloat*)cmd.clearColors[i].float32);
        }
    }

    // TODO: clear depth stencil attachment
}

void gl_command_bind_graphics_pipeline(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_GRAPHICS_PIPELINE);

    const auto& cmd = *(const RCommandBindGraphicsPipeline*)type;
    auto* pipelineObj = (RPipelineGLObj*)cmd.pipeline.unwrap();
    deviceObj->boundGraphicsPipeline = pipelineObj;

    glUseProgram(pipelineObj->gl.handle);
}

static void gl_command_draw(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(*type == RCOMMAND_DRAW);
    LD_ASSERT(deviceObj->boundGraphicsPipeline);

    const auto& cmd = *(const RCommandDraw*)type;
    const GLenum mode = deviceObj->boundGraphicsPipeline->gl.primitiveMode;
    const GLint first = (GLint)cmd.drawInfo.vertexStart;
    const GLsizei count = (GLsizei)cmd.drawInfo.vertexCount;
    const GLsizei instanceCount = (GLsizei)cmd.drawInfo.instanceCount;
    const GLuint baseInstance = (GLuint)cmd.drawInfo.instanceStart;
    glDrawArraysInstancedBaseInstance(mode, first, count, instanceCount, baseInstance);
}

static void gl_command_draw_indexed(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(*type == RCOMMAND_DRAW_INDEXED);
    LD_ASSERT(deviceObj->boundGraphicsPipeline);

    // TODO:
    const auto& cmd = *(const RCommandDrawIndexed*)type;
    const GLenum mode = deviceObj->boundGraphicsPipeline->gl.primitiveMode;
    // const GLenum indexType = ;
    const GLsizei count = (GLsizei)cmd.drawIndexedInfo.indexCount;
    const GLsizei instanceCount = (GLsizei)cmd.drawIndexedInfo.instanceCount;
    const GLuint baseInstance = (GLuint)cmd.drawIndexedInfo.instanceStart;
    // const size_t byteOffset = ;
    // glDrawElementsInstancedBaseInstance(mode, count, indexType, (const void*)byteOffset, instanceCount, baseInstance);
}

static void gl_command_end_pass(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(*type == RCOMMAND_END_PASS);

    (void)type;
    (void)deviceObj;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void gl_command_image_memory_barrier(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    (void)type;
    (void)deviceObj;
}

static void gl_command_copy_image_to_buffer(const RCommandType* type, RDeviceGLObj* deviceObj)
{
    LD_ASSERT(*type == RCOMMAND_COPY_IMAGE_TO_BUFFER);

    const auto& cmd = *(const RCommandCopyImageToBuffer*)type;
    auto* imageObj = (RImageGLObj*)cmd.srcImage.unwrap();
    auto* bufferObj = (RBufferGLObj*)cmd.dstBuffer.unwrap();

    for (const RBufferImageCopy& region : cmd.regions)
    {
        gl_copy_image_to_buffer(imageObj, bufferObj, region);
    }
}

static void gl_copy_image_to_buffer(RImageGLObj* imageObj, RBufferGLObj* bufferObj, const RBufferImageCopy& region)
{
    uint32_t texelSize = RUtil::get_format_texel_size(imageObj->info.format);
    GLenum internalFormat, dataFormat, dataType;
    RUtil::cast_format_gl(imageObj->info.format, internalFormat, dataFormat, dataType);
    uint32_t layerCount = region.imageLayers;
    uint32_t layerSize = region.imageWidth * region.imageHeight * region.imageDepth * texelSize;
    uint32_t copySize = layerSize * layerCount;

    LD_ASSERT(region.bufferOffset + copySize <= bufferObj->info.size);

    bool bufferIsOriginallyMapped = bufferObj->hostMap != nullptr;
    if (!bufferObj->hostMap)
        gl_buffer_map(bufferObj);

    void* dstData = (char*)bufferObj->hostMap + region.bufferOffset;

    constexpr uint32_t mipLevel = 0;

    switch (imageObj->info.type)
    {
    case RIMAGE_TYPE_2D:
        glGetTextureSubImage(imageObj->gl.handle, mipLevel, 0, 0, 0, region.imageWidth, region.imageHeight, region.imageDepth, dataFormat, dataType, copySize, dstData);
        break;
    default:
        LD_UNREACHABLE;
    }

    gl_buffer_unmap(bufferObj); // flush
    if (bufferIsOriginallyMapped)
        gl_buffer_map(bufferObj);
}

} // namespace LD
