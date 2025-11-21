#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>

// OpenGL 4.6 Core Profile
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cstring>
#include <vector>

#include "RBackendObj.h"
#include "RShaderCompiler.h"
#include "RUtilCommon.h"
#include "RUtilGL.h"

namespace LD {

static Log sLog("RBackendGL");

struct RPipelineGLObj;
struct RDeviceGLObj;

/// @brief Attempts to compile OpenGL GLSL of shader type.
/// @return Non-zero shader handle on success, zero otherwise.
static GLuint gl_compile_shader(GLenum glShaderType, const GLchar* glsl);

/// @brief Attempts to link OpenGL program with shaders.
/// @return Non-zero program handle on success, zero otherwise.
static GLuint gl_link_program(size_t shaderCount, GLuint* shaders);

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

    RPipelineGLObj* boundGraphicsPipeline = nullptr;
    RPipelineGLObj* boundComputePipeline = nullptr;
    RIndexType indexType;
};

/// @brief OpenGL shader object.
struct RShaderGLObj : RShaderObj
{
};

/// @brief OpenGL set object.
struct RSetGLObj : RSetObj
{
    struct
    {
        std::vector<std::vector<void*>> bindingSites;
    } gl;
};

static RSet gl_set_pool_allocate(RSetPoolObj* baseSelf, RSetObj* baseSetObj);
static void gl_set_pool_reset(RSetPoolObj* baseSelf);

static constexpr RSetPoolAPI sRSetPoolGLAPI = {
    .allocate = &gl_set_pool_allocate,
    .reset = &gl_set_pool_reset,
};

/// @brief OpenGL set pool object.
struct RSetPoolGLObj : RSetPoolObj
{
    RSetPoolGLObj()
    {
        api = &sRSetPoolGLAPI;
    }
};

/// @brief OpenGL set layout object.
struct RSetLayoutGLObj : RSetLayoutObj
{
};

/// @brief OpenGL pipeline layout object.
struct RPipelineLayoutGLObj : RPipelineLayoutObj
{
    struct
    {
        RShaderOpenGLRemap remap;
    } gl;
};

static void gl_pipeline_create_variant(RPipelineObj* baseObj)
{
    (void)baseObj;
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
        std::vector<GLuint> shaderHandles;
        GLenum primitiveMode;
        GLuint programHandle;
        GLuint vao;
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

static void gl_device_set_pool_ctor(RSetPoolObj* baseObj);
static void gl_device_set_pool_dtor(RSetPoolObj* baseObj);
static RSetPool gl_device_create_set_pool(RDeviceObj* baseSelf, const RSetPoolInfo& setPoolI, RSetPoolObj* baseObj);
static void gl_device_destroy_set_pool(RDeviceObj* baseSelf, RSetPool setPool);

static void gl_device_set_ctor(RSetObj* baseObj);
static void gl_device_set_dtor(RSetObj* baseObj);

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
static RPipeline gl_device_create_compute_pipeline(RDeviceObj* self, const RComputePipelineInfo& pipelineI, RPipelineObj* pipelineObj);
static void gl_device_destroy_pipeline(RDeviceObj* baseSelf, RPipeline pipeline);
static void gl_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI);
static void gl_device_update_set_images(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates);
static void gl_device_update_set_buffers(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates);
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
    .set_pool_ctor = &gl_device_set_pool_ctor,
    .set_pool_dtor = &gl_device_set_pool_dtor,
    .create_set_pool = &gl_device_create_set_pool,
    .destroy_set_pool = &gl_device_destroy_set_pool,
    .set_ctor = &gl_device_set_ctor,
    .set_dtor = &gl_device_set_dtor,
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
    .create_compute_pipeline = &gl_device_create_compute_pipeline,
    .destroy_pipeline = &gl_device_destroy_pipeline,
    .pipeline_variant_pass = &gl_device_pipeline_variant_pass,
    .pipeline_variant_color_write_mask = nullptr,
    .pipeline_variant_depth_test_enable = nullptr,
    .update_set_images = &gl_device_update_set_images,
    .update_set_buffers = &gl_device_update_set_buffers,
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
    RCommandListGLObj* currentList = nullptr;
};

static void gl_copy_buffer(RBufferGLObj* srcBufferObj, RBufferGLObj* dstBufferObj, const RBufferCopy& region);
static void gl_copy_buffer_to_image(RBufferGLObj* bufferObj, RImageGLObj* imageObj, const RBufferImageCopy& region);
static void gl_copy_image_to_buffer(RImageGLObj* imageObj, RBufferGLObj* bufferObj, const RBufferImageCopy& region);
static void gl_bind_set(RPipelineLayoutGLObj* layoutObj, uint32_t setIndex, RSetGLObj* setObj);

static void gl_command_execute(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_begin_pass(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_bind_graphics_pipeline(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_bind_graphics_sets(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_bind_compute_pipeline(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_bind_compute_sets(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_bind_vertex_buffers(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_bind_index_buffer(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_draw(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_draw_indexed(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_draw_indirect(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_draw_indexed_indirect(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_end_pass(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_dispatch(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_image_memory_barrier(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_copy_buffer(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_copy_buffer_to_image(const RCommandType* type, RCommandListGLObj* listObj);
static void gl_command_copy_image_to_buffer(const RCommandType* type, RCommandListGLObj* listObj);

static constexpr void (*sCommandTable[])(const RCommandType*, RCommandListGLObj*) = {
    &gl_command_begin_pass,
    nullptr,
    &gl_command_bind_graphics_pipeline,
    &gl_command_bind_graphics_sets,
    &gl_command_bind_compute_pipeline,
    &gl_command_bind_compute_sets,
    &gl_command_bind_vertex_buffers,
    &gl_command_bind_index_buffer,
    nullptr,
    &gl_command_draw,
    &gl_command_draw_indexed,
    &gl_command_draw_indirect,
    &gl_command_draw_indexed_indirect,
    &gl_command_end_pass,
    &gl_command_dispatch,
    nullptr,
    &gl_command_image_memory_barrier,
    &gl_command_copy_buffer,
    &gl_command_copy_buffer_to_image,
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
    { RTYPE_SET,             sizeof(RSetGLObj) },
    { RTYPE_SET_POOL,        sizeof(RSetPoolGLObj) },
    { RTYPE_PASS,            sizeof(RPassGLObj) },
    { RTYPE_FRAMEBUFFER,     sizeof(RFramebufferGLObj) },
    { RTYPE_PIPELINE_LAYOUT, sizeof(RPipelineLayoutGLObj) },
    { RTYPE_PIPELINE,        sizeof(RPipelineGLObj) },
    { RTYPE_COMMAND_LIST,    sizeof(RCommandListGLObj) },
    { RTYPE_COMMAND_POOL,    sizeof(RCommandPoolGLObj) },
    { RTYPE_QUEUE,           sizeof(RQueueGLObj) },
};
// clang-format on

static GLuint gl_compile_shader(GLenum glShaderType, const GLchar* glsl)
{
    GLuint shaderHandle = glCreateShader(glShaderType);

    GLint len = (GLint)strlen(glsl);
    glShaderSource(shaderHandle, 1, &glsl, &len);
    glCompileShader(shaderHandle);

    GLint status;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE)
    {
        char buf[512];
        GLsizei length;
        glGetShaderInfoLog(shaderHandle, sizeof(buf), &length, buf);
        sLog.error("glCompileShader failed: {}", std::string_view(buf, length));

        glDeleteShader(shaderHandle);
        return (GLuint)0;
    }

    return shaderHandle;
}

static GLuint gl_link_program(size_t shaderCount, GLuint* shaders)
{
    GLuint programHandle = glCreateProgram();

    for (size_t i = 0; i < shaderCount; i++)
        glAttachShader(programHandle, shaders[i]);

    glLinkProgram(programHandle);

    GLint status;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &status);

    if (status != GL_TRUE)
    {
        char buf[512];
        GLsizei length;
        glGetProgramInfoLog(programHandle, sizeof(buf), &length, buf);
        sLog.error("glLinkProgram failed: {}", std::string_view(buf, length));

        glDeleteProgram(programHandle);
        return (GLuint)0;
    }

    return programHandle;
}

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

static RSet gl_set_pool_allocate(RSetPoolObj* baseSelf, RSetObj* baseSetObj)
{
    auto* self = (RSetPoolGLObj*)baseSelf;
    auto* obj = (RSetGLObj*)baseSetObj;

    const uint32_t bindingCount = (uint32_t)self->layoutObj->bindings.size();
    obj->gl.bindingSites.resize(bindingCount);

    for (uint32_t bindingIdx = 0; bindingIdx < bindingCount; bindingIdx++)
    {
        const RSetBindingInfo& bindingI = self->layoutObj->bindings[bindingIdx];
        obj->gl.bindingSites[bindingIdx].resize(bindingI.arrayCount);

        for (uint32_t arrayIdx = 0; arrayIdx < bindingI.arrayCount; arrayIdx++)
            obj->gl.bindingSites[bindingIdx][arrayIdx] = nullptr;
    }

    return RSet(baseSetObj);
}

static void gl_set_pool_reset(RSetPoolObj* baseSelf)
{
    (void)baseSelf;
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
    LD_PROFILE_SCOPE;

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
            gl_command_execute(cmd, listObj);
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

    // extract limits
    GLint glMaxComputeWorkGroupInvocations;
    GLint glMaxComputeWorkGroupCountX, glMaxComputeWorkGroupSizeX;
    GLint glMaxComputeWorkGroupCountY, glMaxComputeWorkGroupSizeY;
    GLint glMaxComputeWorkGroupCountZ, glMaxComputeWorkGroupSizeZ;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &glMaxComputeWorkGroupInvocations);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &glMaxComputeWorkGroupCountX);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &glMaxComputeWorkGroupCountY);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &glMaxComputeWorkGroupCountZ);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &glMaxComputeWorkGroupSizeX);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &glMaxComputeWorkGroupSizeY);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &glMaxComputeWorkGroupSizeZ);
    obj->limits.maxComputeWorkGroupInvocations = (uint32_t)glMaxComputeWorkGroupInvocations;
    obj->limits.maxComputeWorkGroupCount[0] = (uint32_t)glMaxComputeWorkGroupCountX;
    obj->limits.maxComputeWorkGroupCount[1] = (uint32_t)glMaxComputeWorkGroupCountY;
    obj->limits.maxComputeWorkGroupCount[2] = (uint32_t)glMaxComputeWorkGroupCountZ;
    obj->limits.maxComputeWorkGroupSize[0] = (uint32_t)glMaxComputeWorkGroupSizeX;
    obj->limits.maxComputeWorkGroupSize[1] = (uint32_t)glMaxComputeWorkGroupSizeY;
    obj->limits.maxComputeWorkGroupSize[2] = (uint32_t)glMaxComputeWorkGroupSizeZ;
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

    return RShader(obj);
}

static void gl_device_destroy_shader(RDeviceObj* baseSelf, RShader shader)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RShaderGLObj*)shader.unwrap();
}

static void gl_device_set_pool_ctor(RSetPoolObj* baseObj)
{
    auto* obj = (RSetPoolGLObj*)baseObj;

    new (obj) RSetPoolGLObj();
}

static void gl_device_set_pool_dtor(RSetPoolObj* baseObj)
{
    auto* obj = (RSetPoolGLObj*)baseObj;

    obj->~RSetPoolGLObj();
}

static RSetPool gl_device_create_set_pool(RDeviceObj* baseSelf, const RSetPoolInfo& setPoolI, RSetPoolObj* baseObj)
{
    auto* self = (RDeviceGLObj*)baseSelf;

    // TODO:

    return RSetPool(baseObj);
}

static void gl_device_set_ctor(RSetObj* baseObj)
{
    auto* obj = (RSetGLObj*)baseObj;

    new (obj) RSetGLObj();
}

static void gl_device_set_dtor(RSetObj* baseObj)
{
    auto* obj = (RSetGLObj*)baseObj;

    obj->~RSetGLObj();
}

static void gl_device_destroy_set_pool(RDeviceObj* baseSelf, RSetPool setPool)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RSetPoolGLObj*)setPool.unwrap();

    // TODO:
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

    RShaderCompiler compiler;
    bool success = compiler.compute_opengl_remap(baseObj, obj->gl.remap);
    LD_ASSERT(success);
}

static void gl_device_destroy_pipeline_layout(RDeviceObj* baseSelf, RPipelineLayoutObj* baseObj)
{
    (void)baseSelf;
    (void)baseObj;
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
    auto* layoutObj = (RPipelineLayoutGLObj*)obj->layoutObj;

    glCreateVertexArrays(1, &obj->gl.vao);
    glBindVertexArray(obj->gl.vao);

    // vertex attribute description
    uint32_t attrLocation = 0;
    for (const RVertexAttribute& attr : obj->vertexAttributes)
    {
        GLint attrComponentCount;
        GLenum attrComponentType;
        GLuint attrOffset = (GLuint)attr.offset;
        GLuint attrBinding = (GLuint)attr.binding;
        GLboolean normalized = GL_FALSE; // TODO:
        RUtil::cast_glsl_type_gl(attr.type, attrComponentCount, attrComponentType);

        glEnableVertexAttribArray(attrLocation);
        glVertexAttribFormat(attrLocation, attrComponentCount, attrComponentType, normalized, attrOffset);
        glVertexAttribBinding(attrLocation, attrBinding);
        LD_ASSERT(glGetError() == 0);

        // TODO: vertex input such as mat4 would span multiple locations
        attrLocation++;
    }

    // vertex binding description
    for (uint32_t binding = 0; binding < (uint32_t)obj->vertexBindings.size(); binding++)
    {
        GLuint divisor = obj->vertexBindings[binding].inputRate == RBINDING_INPUT_RATE_INSTANCE ? 1 : 0;

        glVertexBindingDivisor(binding, divisor);
    }

    // compile and link shaders into single program
    for (uint32_t i = 0; i < pipelineI.shaderCount; i++)
    {
        RShaderGLObj* shaderObj = (RShaderGLObj*)pipelineI.shaders[i].unwrap();

        RShaderCompiler compiler;
        std::string glsl;
        bool success = compiler.decompile_to_opengl_glsl(layoutObj->gl.remap, shaderObj->spirv, glsl);

        if (!success)
            return {};

        GLenum shaderType;
        RUtil::cast_shader_type_gl(shaderObj->type, shaderType);

        GLuint shaderHandle = gl_compile_shader(shaderType, (const GLchar*)glsl.c_str());
        if (shaderHandle == 0)
            return {};

        obj->gl.shaderHandles.push_back(shaderHandle);
    }

    obj->gl.programHandle = gl_link_program(obj->gl.shaderHandles.size(), obj->gl.shaderHandles.data());
    if (obj->gl.programHandle == 0)
        return {};

    RUtil::cast_primitive_topology_gl(pipelineI.primitiveTopology, obj->gl.primitiveMode);

    return RPipeline(obj);
}

static RPipeline gl_device_create_compute_pipeline(RDeviceObj* baseSelf, const RComputePipelineInfo& pipelineI, RPipelineObj* baseObj)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RPipelineGLObj*)baseObj;
    auto* layoutObj = (RPipelineLayoutGLObj*)obj->layoutObj;
    auto* shaderObj = (RShaderGLObj*)pipelineI.shader.unwrap();

    RShaderCompiler compiler;
    std::string glsl;
    bool success = compiler.decompile_to_opengl_glsl(layoutObj->gl.remap, shaderObj->spirv, glsl);

    if (!success)
        return {};

    GLuint shaderHandle = gl_compile_shader(GL_COMPUTE_SHADER, (const GLchar*)glsl.c_str());
    if (shaderHandle == 0)
        return {};

    obj->gl.shaderHandles.resize(1);
    obj->gl.shaderHandles[0] = shaderHandle;

    obj->gl.programHandle = gl_link_program(1, &shaderHandle);
    if (obj->gl.programHandle == 0)
        return {};

    return RPipeline(obj);
}

static void gl_device_destroy_pipeline(RDeviceObj* baseSelf, RPipeline pipeline)
{
    auto* self = (RDeviceGLObj*)baseSelf;
    auto* obj = (RPipelineGLObj*)pipeline.unwrap();

    for (GLuint shaderHandle : obj->gl.shaderHandles)
        glDeleteShader(shaderHandle);

    obj->gl.shaderHandles.clear();

    glDeleteProgram(obj->gl.programHandle);
    obj->gl.programHandle = 0;

    glDeleteVertexArrays(1, &obj->gl.vao);
    obj->gl.vao = 0;
}

static void gl_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI)
{
    (void)self;
    (void)pipelineObj;
}

static void gl_device_update_set_images(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates)
{
    for (uint32_t i = 0; i < updateCount; i++)
    {
        const RSetImageUpdateInfo& update = updates[i];
        auto* setObj = (RSetGLObj*)update.set.unwrap();

        for (uint32_t j = 0; j < update.imageCount; j++)
        {
            std::vector<void*>& descriptorArray = setObj->gl.bindingSites[update.dstBinding];
            uint32_t arrayIdx = update.dstArrayIndex + j;
            descriptorArray[arrayIdx] = (void*)update.images[j].unwrap();
        }
    }
}

static void gl_device_update_set_buffers(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates)
{
    for (uint32_t i = 0; i < updateCount; i++)
    {
        const RSetBufferUpdateInfo& update = updates[i];
        auto* setObj = (RSetGLObj*)update.set.unwrap();

        for (uint32_t j = 0; j < update.bufferCount; j++)
        {
            std::vector<void*>& descriptorArray = setObj->gl.bindingSites[update.dstBinding];
            uint32_t arrayIdx = update.dstArrayIndex + j;
            descriptorArray[arrayIdx] = (void*)update.buffers[j].unwrap();
        }
    }
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

static void gl_command_execute(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(sCommandTable[(int)*type]);

    sCommandTable[(int)*type](type, listObj);
}

void gl_command_begin_pass(const RCommandType* type, RCommandListGLObj* listObj)
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

void gl_command_bind_graphics_pipeline(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_GRAPHICS_PIPELINE);

    const auto& cmd = *(const RCommandBindGraphicsPipeline*)type;
    auto* pipelineObj = (RPipelineGLObj*)cmd.pipeline.unwrap();
    listObj->boundGraphicsPipeline = pipelineObj;

    glBindVertexArray(pipelineObj->gl.vao);
    glUseProgram(pipelineObj->gl.programHandle);
}

static void gl_command_bind_graphics_sets(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_GRAPHICS_SETS);
    LD_ASSERT(listObj->boundGraphicsPipeline);

    const auto& cmd = *(const RCommandBindGraphicsSets*)type;
    auto* layoutObj = (RPipelineLayoutGLObj*)listObj->boundGraphicsPipeline->layoutObj;

    for (uint32_t i = 0; i < (uint32_t)cmd.sets.size(); i++)
    {
        RSetGLObj* setObj = (RSetGLObj*)cmd.sets[i].unwrap();
        uint32_t setIdx = cmd.firstSet + i;

        gl_bind_set(layoutObj, setIdx, setObj);
    }
}

static void gl_command_bind_compute_pipeline(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_COMPUTE_PIPELINE);

    const auto& cmd = *(const RCommandBindComputePipeline*)type;
    auto* pipelineObj = (RPipelineGLObj*)cmd.pipeline.unwrap();
    listObj->boundComputePipeline = pipelineObj;

    glUseProgram(pipelineObj->gl.programHandle);
}

static void gl_command_bind_compute_sets(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_COMPUTE_SETS);
    LD_ASSERT(listObj->boundComputePipeline);

    const auto& cmd = *(const RCommandBindComputeSets*)type;
    auto* layoutObj = (RPipelineLayoutGLObj*)listObj->boundComputePipeline->layoutObj;

    for (uint32_t i = 0; i < (uint32_t)cmd.sets.size(); i++)
    {
        RSetGLObj* setObj = (RSetGLObj*)cmd.sets[i].unwrap();
        uint32_t setIdx = cmd.firstSet + i;

        gl_bind_set(layoutObj, setIdx, setObj);
    }
}

static void gl_command_bind_vertex_buffers(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_VERTEX_BUFFERS);

    const auto& cmd = *(const RCommandBindVertexBuffers*)type;

    for (uint32_t i = 0; i < (uint32_t)cmd.buffers.size(); i++)
    {
        uint32_t bindingIndex = cmd.firstBinding + i;

        LD_ASSERT(bindingIndex < listObj->boundGraphicsPipeline->vertexBindings.size());
        GLsizei vertexStride = (GLsizei)listObj->boundGraphicsPipeline->vertexBindings[bindingIndex].stride;
        auto* bufferObj = (RBufferGLObj*)cmd.buffers[i].unwrap();

        glBindVertexBuffer(bindingIndex, bufferObj->gl.handle, 0, vertexStride);
        LD_ASSERT(glGetError() == 0);
    }
}

static void gl_command_bind_index_buffer(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_BIND_INDEX_BUFFER);

    const auto& cmd = *(const RCommandBindIndexBuffer*)type;
    auto* bufferObj = (RBufferGLObj*)cmd.buffer.unwrap();

    // IBO index type is required later for indexed draw calls.
    listObj->indexType = cmd.indexType;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObj->gl.handle);
    LD_ASSERT(glGetError() == 0);
}

static void gl_command_draw(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_DRAW);
    LD_ASSERT(listObj->boundGraphicsPipeline);

    const auto& cmd = *(const RCommandDraw*)type;
    const GLenum mode = listObj->boundGraphicsPipeline->gl.primitiveMode;
    const GLint first = (GLint)cmd.drawInfo.vertexStart;
    const GLsizei count = (GLsizei)cmd.drawInfo.vertexCount;
    const GLsizei instanceCount = (GLsizei)cmd.drawInfo.instanceCount;
    const GLuint baseInstance = (GLuint)cmd.drawInfo.instanceStart;
    glDrawArraysInstancedBaseInstance(mode, first, count, instanceCount, baseInstance);
    LD_ASSERT(glGetError() == 0);
}

static void gl_command_draw_indexed(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_DRAW_INDEXED);
    LD_ASSERT(listObj->boundGraphicsPipeline); // missing graphics pipeline

    size_t indexByteSize;
    GLenum glIndexType;
    RUtil::cast_index_type_gl(listObj->indexType, glIndexType, indexByteSize);

    const auto& cmd = *(const RCommandDrawIndexed*)type;
    const GLenum mode = listObj->boundGraphicsPipeline->gl.primitiveMode;
    const GLsizei count = (GLsizei)cmd.drawIndexedInfo.indexCount;
    const size_t byteOffset = indexByteSize * cmd.drawIndexedInfo.indexStart;
    const GLsizei instanceCount = (GLsizei)cmd.drawIndexedInfo.instanceCount;
    const GLint baseVertex = (GLint)cmd.drawIndexedInfo.vertexOffset;
    const GLuint baseInstance = (GLuint)cmd.drawIndexedInfo.instanceStart;
    glDrawElementsInstancedBaseVertexBaseInstance(mode, count, glIndexType, (const void*)byteOffset, instanceCount, baseVertex, baseInstance);
    LD_ASSERT(glGetError() == 0);
}

static void gl_command_draw_indirect(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_DRAW_INDIRECT);
    LD_ASSERT(listObj->boundGraphicsPipeline); // missing graphics pipeline

    const auto& cmd = *(const RCommandDrawIndirect*)type;

    auto* bufferObj = (RBufferGLObj*)cmd.drawIndirectInfo.indirectBuffer.unwrap();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bufferObj->gl.handle);
    LD_ASSERT(glGetError() == 0);

    const GLenum mode = listObj->boundGraphicsPipeline->gl.primitiveMode;
    const GLsizei drawCount = (GLsizei)cmd.drawIndirectInfo.infoCount;
    const GLsizei stride = (GLsizei)cmd.drawIndirectInfo.stride;
    glMultiDrawArraysIndirect(mode, (const void*)cmd.drawIndirectInfo.offset, drawCount, stride);
    LD_ASSERT(glGetError() == 0);
}

static void gl_command_draw_indexed_indirect(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_DRAW_INDEXED_INDIRECT);
    LD_ASSERT(listObj->boundGraphicsPipeline); // missing graphics pipeline

    const auto& cmd = *(const RCommandDrawIndexedIndirect*)type;

    size_t indexByteSize;
    GLenum glIndexType;
    RUtil::cast_index_type_gl(listObj->indexType, glIndexType, indexByteSize);

    auto* bufferObj = (RBufferGLObj*)cmd.drawIndexedIndirectInfo.indirectBuffer.unwrap();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bufferObj->gl.handle);
    LD_ASSERT(glGetError() == 0);

    const GLenum mode = listObj->boundGraphicsPipeline->gl.primitiveMode;
    const GLsizei drawCount = (GLsizei)cmd.drawIndexedIndirectInfo.infoCount;
    const GLsizei stride = (GLsizei)cmd.drawIndexedIndirectInfo.stride;
    glMultiDrawElementsIndirect(mode, glIndexType, (const void*)cmd.drawIndexedIndirectInfo.offset, drawCount, stride);
    LD_ASSERT(glGetError() == 0);
}

static void gl_command_end_pass(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_END_PASS);

    (void)type;
    (void)listObj;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void gl_command_dispatch(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_DISPATCH);

    const auto& cmd = *(const RCommandDispatch*)type;

    glDispatchCompute((GLuint)cmd.groupCountX, (GLuint)cmd.groupCountY, (GLuint)cmd.groupCountZ);
}

static void gl_command_image_memory_barrier(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_IMAGE_MEMORY_BARRIER);

    (void)type;
    (void)listObj;
}

static void gl_command_copy_buffer(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_COPY_BUFFER);

    const auto& cmd = *(const RCommandCopyBuffer*)type;
    auto* srcBufferObj = (RBufferGLObj*)cmd.srcBuffer.unwrap();
    auto* dstBufferObj = (RBufferGLObj*)cmd.dstBuffer.unwrap();

    for (const RBufferCopy& region : cmd.regions)
    {
        gl_copy_buffer(srcBufferObj, dstBufferObj, region);
    }
}

static void gl_command_copy_buffer_to_image(const RCommandType* type, RCommandListGLObj* listObj)
{
    LD_ASSERT(*type == RCOMMAND_COPY_BUFFER_TO_IMAGE);

    const auto& cmd = *(const RCommandCopyBufferToImage*)type;
    auto* imageObj = (RImageGLObj*)cmd.dstImage.unwrap();
    auto* bufferObj = (RBufferGLObj*)cmd.srcBuffer.unwrap();

    for (const RBufferImageCopy& region : cmd.regions)
    {
        gl_copy_buffer_to_image(bufferObj, imageObj, region);
    }
}

static void gl_command_copy_image_to_buffer(const RCommandType* type, RCommandListGLObj* listObj)
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

static void gl_copy_buffer(RBufferGLObj* srcBufferObj, RBufferGLObj* dstBufferObj, const RBufferCopy& region)
{
    GLintptr readOffset = (GLintptr)region.srcOffset;
    GLintptr writeOffset = (GLintptr)region.dstOffset;
    GLsizeiptr copySize = (GLsizeiptr)region.size;

    LD_ASSERT(readOffset + copySize <= srcBufferObj->info.size);
    LD_ASSERT(writeOffset + copySize <= dstBufferObj->info.size);

    glCopyNamedBufferSubData(srcBufferObj->gl.handle, dstBufferObj->gl.handle, readOffset, writeOffset, copySize);
}

static void gl_copy_buffer_to_image(RBufferGLObj* bufferObj, RImageGLObj* imageObj, const RBufferImageCopy& region)
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

    void* srcData = (char*)bufferObj->hostMap + region.bufferOffset;

    constexpr uint32_t mipLevel = 0;

    switch (imageObj->info.type)
    {
    case RIMAGE_TYPE_2D:
        glTextureSubImage2D(imageObj->gl.handle, mipLevel, 0, 0, region.imageWidth, region.imageHeight, dataFormat, dataType, srcData);
        break;
    default:
        LD_UNREACHABLE;
    }

    if (!bufferIsOriginallyMapped)
        gl_buffer_unmap(bufferObj);
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

static void gl_bind_set(RPipelineLayoutGLObj* layoutObj, uint32_t setIndex, RSetGLObj* setObj)
{
    RSetLayoutGLObj* setLayoutObj = (RSetLayoutGLObj*)layoutObj->setLayoutObjs[setIndex];
    uint32_t bindingCount = (uint32_t)setLayoutObj->bindings.size();

    for (uint32_t bindingIdx = 0; bindingIdx < bindingCount; bindingIdx++)
    {
        const RSetBindingInfo& bindingI = setLayoutObj->bindings[bindingIdx];
        const RShaderOpenGLBindingRemap* remap = layoutObj->gl.remap.get_binding_remap(setIndex, bindingIdx);

        // TODO: error control flow
        LD_ASSERT(remap);

        // TODO: array of samplers
        LD_ASSERT(bindingI.arrayCount == 1);

        RImageGLObj* imageObj = nullptr;
        RBufferGLObj* bufferObj = nullptr;

        switch (bindingI.type)
        {
        case RBINDING_TYPE_COMBINED_IMAGE_SAMPLER:
            imageObj = (RImageGLObj*)setObj->gl.bindingSites[bindingIdx][0];
            if (imageObj)
            {
                glActiveTexture(GL_TEXTURE0 + remap->glBindingIndex);
                glBindTexture(imageObj->gl.target, imageObj->gl.handle);
            }
            break;
        case RBINDING_TYPE_UNIFORM_BUFFER:
            bufferObj = (RBufferGLObj*)setObj->gl.bindingSites[bindingIdx][0];
            if (bufferObj)
            {
                glBindBufferBase(GL_UNIFORM_BUFFER, remap->glBindingIndex, bufferObj->gl.handle);
            }
            break;
        case RBINDING_TYPE_STORAGE_BUFFER:
            bufferObj = (RBufferGLObj*)setObj->gl.bindingSites[bindingIdx][0];
            if (bufferObj)
            {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, remap->glBindingIndex, bufferObj->gl.handle);
            }
            break;
        case RBINDING_TYPE_STORAGE_IMAGE:  // TODO:
        default:
            LD_UNREACHABLE;
        }
    }
}

} // namespace LD
