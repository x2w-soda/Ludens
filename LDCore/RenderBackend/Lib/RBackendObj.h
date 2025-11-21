#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "RCommand.h"
#include "RData.h"
#include "RShaderCompiler.h"

// RBackendObj.h
// - internal header defining base object struct and their API (vtable)
// - this header should still be graphics API agnostic

#define PIPELINE_LAYOUT_MAX_RESOURCE_SETS 4

struct GLFWwindow;

namespace LD {

struct RCommandPoolObj;
struct RPipelineLayoutObj;
struct RDeviceObj;

/// @brief Render backend types.
enum RType
{
    RTYPE_DEVICE,
    RTYPE_SEMAPHORE,
    RTYPE_FENCE,
    RTYPE_BUFFER,
    RTYPE_IMAGE,
    RTYPE_SHADER,
    RTYPE_SET_LAYOUT,
    RTYPE_SET,
    RTYPE_SET_POOL,
    RTYPE_PASS,
    RTYPE_FRAMEBUFFER,
    RTYPE_PIPELINE_LAYOUT,
    RTYPE_PIPELINE,
    RTYPE_COMMAND_LIST,
    RTYPE_COMMAND_POOL,
    RTYPE_QUEUE,
    RTYPE_ENUM_COUNT,
};

struct RObjectID
{
    // NOTE: not atomic, currently only main thread
    //       may create and destroy objects
    static uint64_t sCounter;

    inline static uint64_t get() { return sCounter++; }
};

struct RBufferAPI
{
    void (*map)(RBufferObj* self);
    void* (*map_read)(RBufferObj* self, uint64_t offset, uint64_t size);
    void (*map_write)(RBufferObj* self, uint64_t offset, uint64_t size, const void* data);
    void (*unmap)(RBufferObj* self);
};

/// @brief Base buffer object.
struct RBufferObj
{
    const RBufferAPI* api;
    uint64_t rid;
    RDevice device;
    RBufferInfo info;
    void* hostMap;
};

/// @brief Base image object.
struct RImageObj
{
    uint64_t rid;
    RDevice device;
    RImageInfo info;
    std::unordered_set<uint32_t> fboHashes;
};

/// @brief Base render pass object.
struct RPassObj
{
    uint64_t rid;
    uint32_t hash;
    uint32_t colorAttachmentCount;
    RSampleCountBit samples;
    bool hasDepthStencilAttachment;
};

// NOTE: Framebuffer is managed internally by the render backend,
//       user does not manage framebuffer lifetimes. While the
//       backend graphics API does need to create and invalidate
//       framebuffers, the user may lazily begin render passes and
//       recreate images at will.
struct RFramebufferInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t colorAttachmentCount;
    RImage* colorAttachments;
    RImage* colorResolveAttachments;
    RImage depthStencilAttachment;
    RPassInfo pass;
};

uint32_t hash32_framebuffer_info(const RFramebufferInfo& framebufferI);

struct RFramebufferObj
{
    uint64_t rid;
    uint32_t hash;
    uint32_t width;
    uint32_t height;
    RPassObj* passObj;
};

struct RCommandListAPI
{
    void (*begin)(RCommandListObj* self, bool oneTimeSubmit);
    void (*end)(RCommandListObj* self);
    void (*reset)(RCommandListObj* self);
    void (*cmd_begin_pass)(RCommandListObj* self, const RPassBeginInfo& passBI, RFramebufferObj* framebufferObj);
    void (*cmd_push_constant)(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t offset, uint32_t size, const void* data);
    void (*cmd_bind_graphics_pipeline)(RCommandListObj* self, RPipeline pipeline);
    void (*cmd_bind_graphics_sets)(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t firstSet, uint32_t setCount, RSet* sets);
    void (*cmd_bind_compute_pipeline)(RCommandListObj* self, RPipeline pipeline);
    void (*cmd_bind_compute_sets)(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t firstSet, uint32_t setCount, RSet* sets);
    void (*cmd_bind_vertex_buffers)(RCommandListObj* self, uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);
    void (*cmd_bind_index_buffer)(RCommandListObj* self, RBuffer buffer, RIndexType indexType);
    void (*cmd_dispatch)(RCommandListObj* self, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void (*cmd_set_scissor)(RCommandListObj* self, const Rect& scissor);
    void (*cmd_draw)(RCommandListObj* self, const RDrawInfo& drawI);
    void (*cmd_draw_indexed)(RCommandListObj* self, const RDrawIndexedInfo& drawI);
    void (*cmd_draw_indirect)(RCommandListObj* self, const RDrawIndirectInfo& drawI);
    void (*cmd_draw_indexed_indirect)(RCommandListObj* self, const RDrawIndexedIndirectInfo& drawI);
    void (*cmd_end_pass)(RCommandListObj* self);
    void (*cmd_buffer_memory_barrier)(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier);
    void (*cmd_image_memory_barrier)(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);
    void (*cmd_copy_buffer)(RCommandListObj* self, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);
    void (*cmd_copy_buffer_to_image)(RCommandListObj* self, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);
    void (*cmd_copy_image_to_buffer)(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions);
    void (*cmd_blit_image)(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter);
};

/// @brief Base command list object.
struct RCommandListObj
{
    const RCommandListAPI* api;
    uint64_t rid;
    RDeviceObj* deviceObj;
    RCommandPoolObj* poolObj; /// the command pool allocated from
    RPassInfoData currentPass;
    std::vector<const RCommandType*> captures;
    LinearAllocator captureLA = {};
};

struct RCommandPoolAPI
{
    RCommandList (*allocate)(RCommandPoolObj* self, RCommandListObj* listObj);
    void (*reset)(RCommandPoolObj* self);
};

/// @brief Base command pool object.
struct RCommandPoolObj
{
    const RCommandPoolAPI* api;
    uint64_t rid;
    std::vector<RCommandList> lists;
    RDeviceObj* deviceObj;
    bool hintTransient;
    bool listResettable;
};

/// @brief Base shader object.
struct RShaderObj
{
    uint64_t rid;
    RShaderType type;
    RShaderReflection reflection;
    std::vector<uint32_t> spirv;
};

/// @brief Base set layout object.
struct RSetLayoutObj
{
    uint64_t rid;
    uint32_t hash;
    RDeviceObj* deviceObj;
    std::vector<RSetBindingInfo> bindings;
};

/// @brief Base set object.
struct RSetObj
{
    uint64_t rid; // TODO: unused
};

struct RSetPoolAPI
{
    RSet (*allocate)(RSetPoolObj* self, RSetObj* setObj);
    void (*reset)(RSetPoolObj* self);
};

/// @brief Base set pool object.
struct RSetPoolObj
{
    const RSetPoolAPI* api;
    uint64_t rid;
    LinearAllocator setLA;
    RDeviceObj* deviceObj;
    RSetLayoutObj* layoutObj;
    std::vector<RSet> sets;
};

/// @brief Base pipeline layout object.
struct RPipelineLayoutObj
{
    uint64_t rid;
    uint32_t hash;
    uint32_t setCount;
    RSetLayoutObj* setLayoutObjs[PIPELINE_LAYOUT_MAX_RESOURCE_SETS];
};

struct RPipelineAPI
{
    void (*create_variant)(RPipelineObj* self);
};

/// @brief Base pipeline object.
struct RPipelineObj
{
    const RPipelineAPI* api;
    uint64_t rid;
    RDeviceObj* deviceObj;
    RPipelineLayoutObj* layoutObj;
    std::vector<RVertexBinding> vertexBindings;
    std::vector<RVertexAttribute> vertexAttributes;

    struct
    {
        bool depthTestEnabled;
        RPassObj* passObj;
        std::vector<RColorComponentFlags> colorWriteMasks;
    } variant;
};

struct RQueueAPI
{
    void (*wait_idle)(RQueueObj* self);
    void (*submit)(RQueueObj* self, const RSubmitInfo& submitI, RFence fence);
};

/// @brief Base queue object.
struct RQueueObj
{
    const RQueueAPI* api;
};

/// @brief Base semaphore object.
struct RSemaphoreObj
{
    uint64_t rid;
};

/// @brief Base fence object.
struct RFenceObj
{
    uint64_t rid;
};

struct RDeviceAPI
{
    size_t (*get_obj_size)(RType type);

    void (*semaphore_ctor)(RSemaphoreObj* semaphoreObj);
    void (*semaphore_dtor)(RSemaphoreObj* semaphoreObj);
    RSemaphore (*create_semaphore)(RDeviceObj* self, RSemaphoreObj* semaphoreObj);
    void (*destroy_semaphore)(RDeviceObj* self, RSemaphore semaphore);

    void (*fence_ctor)(RFenceObj* fenceObj);
    void (*fence_dtor)(RFenceObj* fenceObj);
    RFence (*create_fence)(RDeviceObj* self, bool createSignaled, RFenceObj* fenceObj);
    void (*destroy_fence)(RDeviceObj* self, RFence fence);

    void (*buffer_ctor)(RBufferObj* bufferObj);
    void (*buffer_dtor)(RBufferObj* bufferObj);
    RBuffer (*create_buffer)(RDeviceObj* self, const RBufferInfo& bufferI, RBufferObj* bufferObj);
    void (*destroy_buffer)(RDeviceObj* self, RBuffer buffer);

    void (*image_ctor)(RImageObj* imageObj);
    void (*image_dtor)(RImageObj* imageObj);
    RImage (*create_image)(RDeviceObj* self, const RImageInfo& imageI, RImageObj* imageObj);
    void (*destroy_image)(RDeviceObj* self, RImage image);

    void (*pass_ctor)(RPassObj* passObj);
    void (*pass_dtor)(RPassObj* passObj);
    void (*create_pass)(RDeviceObj* self, const RPassInfo& passI, RPassObj* passObj);
    void (*destroy_pass)(RDeviceObj* self, RPassObj* passObj);

    void (*framebuffer_ctor)(RFramebufferObj* framebufferObj);
    void (*framebuffer_dtor)(RFramebufferObj* framebufferObj);
    void (*create_framebuffer)(RDeviceObj* self, const RFramebufferInfo& fbI, RFramebufferObj* framebufferObj);
    void (*destroy_framebuffer)(RDeviceObj* self, RFramebufferObj* framebufferObj);

    void (*command_pool_ctor)(RCommandPoolObj* commandPoolObj);
    void (*command_pool_dtor)(RCommandPoolObj* commandPoolObj);
    RCommandPool (*create_command_pool)(RDeviceObj* self, const RCommandPoolInfo& poolI, RCommandPoolObj* poolObj);
    void (*destroy_command_pool)(RDeviceObj* self, RCommandPool pool);

    void (*command_list_ctor)(RCommandListObj* commandListObj);
    void (*command_list_dtor)(RCommandListObj* commandListObj);

    void (*shader_ctor)(RShaderObj* shaderObj);
    void (*shader_dtor)(RShaderObj* shaderObj);
    RShader (*create_shader)(RDeviceObj* self, const RShaderInfo& shaderI, RShaderObj* shaderObj);
    void (*destroy_shader)(RDeviceObj* self, RShader shader);

    void (*set_pool_ctor)(RSetPoolObj* setPoolObj);
    void (*set_pool_dtor)(RSetPoolObj* setPoolObj);
    RSetPool (*create_set_pool)(RDeviceObj* self, const RSetPoolInfo& poolI, RSetPoolObj* poolObj);
    void (*destroy_set_pool)(RDeviceObj* self, RSetPool pool);

    void (*set_ctor)(RSetObj* setObj);
    void (*set_dtor)(RSetObj* setObj);

    void (*set_layout_ctor)(RSetLayoutObj* setLayoutObj);
    void (*set_layout_dtor)(RSetLayoutObj* setLayoutObj);
    void (*create_set_layout)(RDeviceObj* self, const RSetLayoutInfo& layoutI, RSetLayoutObj* layoutObj);
    void (*destroy_set_layout)(RDeviceObj* self, RSetLayoutObj* layout);

    void (*pipeline_layout_ctor)(RPipelineLayoutObj* layoutObj);
    void (*pipeline_layout_dtor)(RPipelineLayoutObj* layoutObj);
    void (*create_pipeline_layout)(RDeviceObj* self, const RPipelineLayoutInfo& layoutI, RPipelineLayoutObj* layoutObj);
    void (*destroy_pipeline_layout)(RDeviceObj* self, RPipelineLayoutObj* layoutObj);

    void (*pipeline_ctor)(RPipelineObj* pipelineObj);
    void (*pipeline_dtor)(RPipelineObj* pipelineObj);
    RPipeline (*create_pipeline)(RDeviceObj* self, const RPipelineInfo& pipelineI, RPipelineObj* pipelineObj);
    RPipeline (*create_compute_pipeline)(RDeviceObj* self, const RComputePipelineInfo& pipelineI, RPipelineObj* pipelineObj);
    void (*destroy_pipeline)(RDeviceObj* self, RPipeline pipeline);

    void (*pipeline_variant_pass)(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI);
    void (*pipeline_variant_color_write_mask)(RDeviceObj* self, RPipelineObj* pipelineObj, uint32_t index, RColorComponentFlags mask);
    void (*pipeline_variant_depth_test_enable)(RDeviceObj* self, RPipelineObj* pipelineObj, bool enable);

    void (*update_set_images)(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates);
    void (*update_set_buffers)(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates);

    uint32_t (*next_frame)(RDeviceObj* self, RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete);
    void (*present_frame)(RDeviceObj* self);

    void (*get_depth_stencil_formats)(RDeviceObj* self, RFormat* format, uint32_t& count);
    RSampleCountBit (*get_max_sample_count)(RDeviceObj* self);
    RFormat (*get_swapchain_color_format)(RDeviceObj* self);
    RImage (*get_swapchain_color_attachment)(RDeviceObj* self, uint32_t frameIdx);
    uint32_t (*get_swapchain_image_count)(RDeviceObj* self);
    void (*get_swapchain_extent)(RDeviceObj* self, uint32_t* width, uint32_t* height);
    uint32_t (*get_frames_in_flight_count)(RDeviceObj* self);
    RQueue (*get_graphics_queue)(RDeviceObj* self);
    void (*wait_idle)(RDeviceObj* self);
};

/// @brief Base render device object.
struct RDeviceObj
{
    const RDeviceAPI* api;
    uint64_t rid;
    uint32_t frameIndex;
    RDeviceBackend backend;
    GLFWwindow* glfw = nullptr;
    bool isHeadless;

    RPassObj* get_or_create_pass_obj(const RPassInfo& passI);
    RSetLayoutObj* get_or_create_set_layout_obj(const RSetLayoutInfo& layoutI);
    RPipelineLayoutObj* get_or_create_pipeline_layout_obj(const RPipelineLayoutInfo& layoutI);
    RFramebufferObj* get_or_create_framebuffer_obj(const RFramebufferInfo& framebufferI);
};

size_t vk_device_byte_size();
void vk_device_ctor(RDeviceObj* obj);
void vk_device_dtor(RDeviceObj* obj);
void vk_create_device(struct RDeviceObj* obj, const RDeviceInfo& info);
void vk_destroy_device(struct RDeviceObj* obj);

size_t gl_device_byte_size();
void gl_device_ctor(RDeviceObj* obj);
void gl_device_dtor(RDeviceObj* obj);
void gl_create_device(struct RDeviceObj* obj, const RDeviceInfo& info);
void gl_destroy_device(struct RDeviceObj* obj);

} // namespace LD
