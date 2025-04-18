#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>
#include <vector>
#include <vk_mem_alloc.h>    // hide from user
#include <vulkan/vulkan.hpp> // hide from user

#define PIPELINE_LAYOUT_MAX_RESOURCE_SETS 4

namespace LD {

/// @brief Vulkan physical device properties
struct PhysicalDevice
{
    VkPhysicalDevice handle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProps;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkSurfaceCapabilitiesKHR surfaceCaps;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkFormat> depthStencilFormats; /// formats with VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    std::vector<VkQueueFamilyProperties> familyProps;
    std::vector<VkPresentModeKHR> presentModes;
};

/// @brief information to create a Vulkan Swapchain
struct SwapchainInfo
{
    VkFormat imageFormat;
    VkFormat depthStencilFormat;
    VkPresentModeKHR presentMode;
    VkColorSpaceKHR imageColorSpace;
};

/// @brief Vulkan Swapchain
struct Swapchain
{
    VkSwapchainKHR handle;
    SwapchainInfo info;
    std::vector<VkImage> images; // external resource owned by VkSwapchainKHR
    std::vector<RImage> colorAttachments;
    uint32_t width;
    uint32_t height;
};

struct RBufferObj
{
    RDevice device;
    RBufferInfo info;
    void* hostMap;

    void (*map)(RBufferObj* self);
    void (*map_write)(RBufferObj* self, uint64_t offset, uint64_t size, const void* data);
    void (*unmap)(RBufferObj* self);

    void init_vk_api();

    struct
    {
        VmaAllocation vma;
        VkBuffer handle;
    } vk;
};

struct RImageObj
{
    RDevice device;
    RImageInfo info;

    struct
    {
        VmaAllocation vma;
        VkImage handle;
        VkImageView viewHandle;
        VkSampler samplerHandle;
    } vk;
};

struct RPassObj
{
    uint32_t hash;
    uint32_t colorAttachmentCount;
    bool hasDepthStencilAttachment;

    struct
    {
        VkRenderPass handle;
    } vk;
};

struct RFramebufferObj
{
    uint32_t width;
    uint32_t height;

    struct
    {
        VkFramebuffer handle;
    } vk;
};

struct RCommandListObj
{
    void (*free)(RCommandListObj* self);
    void (*begin)(RCommandListObj* self, bool oneTimeSubmit);
    void (*end)(RCommandListObj* self);
    void (*cmd_begin_pass)(RCommandListObj* self, const RPassBeginInfo& passBI);
    void (*cmd_bind_graphics_pipeline)(RCommandListObj* self, RPipeline pipeline);
    void (*cmd_bind_graphics_sets)(RCommandListObj* self, RPipelineLayout layout, uint32_t firstSet, uint32_t setCount, RSet* sets);
    void (*cmd_bind_vertex_buffers)(RCommandListObj* self, uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);
    void (*cmd_bind_index_buffer)(RCommandListObj* self, RBuffer buffer, RIndexType indexType);
    void (*cmd_draw)(RCommandListObj* self, const RDrawInfo& drawI);
    void (*cmd_draw_indexed)(RCommandListObj* self, const RDrawIndexedInfo& drawI);
    void (*cmd_end_pass)(RCommandListObj* self);
    void (*cmd_image_memory_barrier)(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);
    void (*cmd_copy_buffer)(RCommandListObj* self, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);
    void (*cmd_copy_buffer_to_image)(RCommandListObj* self, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);

    void init_vk_api();

    RCommandPoolObj* poolObj; /// the command pool allocated from

    struct
    {
        VkDevice device;
        VkCommandBuffer handle;
    } vk;
};

struct RCommandPoolObj
{
    RCommandList (*allocate)(RCommandPoolObj* self);

    void init_vk_api();

    uint32_t commandBufferCount; /// number of command buffers allocated and not yet freed

    struct
    {
        VkDevice device;
        VkCommandPool handle;
    } vk;
};

struct RShaderObj
{
    RShaderType type;

    struct
    {
        VkShaderModule handle;
    } vk;
};

struct RSetLayoutObj
{
    uint32_t hash;

    struct
    {
        VkDescriptorSetLayout handle;
    } vk;
};

struct RSetObj
{
    struct
    {
        VkDescriptorSet handle;
    } vk;
};

struct RSetPoolObj
{
    LinearAllocator setLA;

    RSet (*allocate)(RSetPoolObj* self, RSetLayout layout, RSetObj* setObj);
    void (*reset)(RSetPoolObj* self);

    void init_vk_api();

    struct
    {
        VkDevice device;
        VkDescriptorPool handle;
    } vk;
};

struct RPipelineLayoutObj
{
    uint32_t hash;
    uint32_t set_count;
    RSetLayout set_layouts[PIPELINE_LAYOUT_MAX_RESOURCE_SETS];

    struct
    {
        VkPipelineLayout handle;
    } vk;
};

struct RPipelineObj
{
    RPipelineLayout layout;

    struct
    {
        VkPipeline handle;
    } vk;
};

struct RQueueObj
{
    void (*wait_idle)(RQueueObj* self);
    void (*submit)(RQueueObj* self, const RSubmitInfo& submitI, RFence fence);

    void init_vk_api();

    struct
    {
        uint32_t familyIdx;
        VkQueue handle;
    } vk;
};

struct RSemaphoreObj
{
    struct
    {
        VkSemaphore handle;
    } vk;
};

struct RFenceObj
{
    struct
    {
        VkFence handle;
    } vk;
};

struct RDeviceObj
{
    RSemaphore (*create_semaphore)(RDeviceObj* self);
    void (*destroy_semaphore)(RDeviceObj* self, RSemaphore semaphore);

    RFence (*create_fence)(RDeviceObj* self, bool createSignaled);
    void (*destroy_fence)(RDeviceObj* self, RFence fence);

    RBuffer (*create_buffer)(RDeviceObj* self, const RBufferInfo& bufferI);
    void (*destroy_buffer)(RDeviceObj* self, RBuffer buffer);

    RImage (*create_image)(RDeviceObj* self, const RImageInfo& imageI);
    void (*destroy_image)(RDeviceObj* self, RImage image);

    RPass (*create_pass)(RDeviceObj* self, const RPassInfo& passI);
    void (*destroy_pass)(RDeviceObj* self, RPass pass);

    RFramebuffer (*create_framebuffer)(RDeviceObj* self, const RFramebufferInfo& fbI);
    void (*destroy_framebuffer)(RDeviceObj* self, RFramebuffer fb);

    RCommandPool (*create_command_pool)(RDeviceObj* self, const RCommandPoolInfo& poolI);
    void (*destroy_command_pool)(RDeviceObj* self, RCommandPool pool);

    RShader (*create_shader)(RDeviceObj* self, const RShaderInfo& shaderI);
    void (*destroy_shader)(RDeviceObj* self, RShader shader);

    RSetPool (*create_set_pool)(RDeviceObj* self, const RSetPoolInfo& poolI);
    void (*destroy_set_pool)(RDeviceObj* self, RSetPool pool);

    RSetLayout (*create_set_layout)(RDeviceObj* self, const RSetLayoutInfo& layoutI);
    void (*destroy_set_layout)(RDeviceObj* self, RSetLayout layout);

    RPipelineLayout (*create_pipeline_layout)(RDeviceObj* self, const RPipelineLayoutInfo& layoutI);
    void (*destroy_pipeline_layout)(RDeviceObj* self, RPipelineLayout layout);

    RPipeline (*create_pipeline)(RDeviceObj* self, const RPipelineInfo& pipelineI);
    void (*destroy_pipeline)(RDeviceObj* self, RPipeline pipeline);

    void (*update_set_images)(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates);

    void (*update_set_buffers)(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates);

    uint32_t (*next_frame)(RDeviceObj* self, RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete);
    void (*present_frame)(RDeviceObj* self);
    RImage (*get_swapchain_color_attachment)(RDeviceObj* self, uint32_t frameIdx);
    uint32_t (*get_swapchain_image_count)(RDeviceObj* self);
    uint32_t (*get_frames_in_flight_count)(RDeviceObj* self);
    RQueue (*get_graphics_queue)(RDeviceObj* self);

    void init_vk_api();

    RDeviceBackend backend;

    struct
    {
        VmaAllocator vma;
        VkInstance instance;
        VkSurfaceKHR surface;
        PhysicalDevice pdevice;
        Swapchain swapchain;
        VkDevice device;
        uint32_t frameIdx;
        uint32_t imageIdx;
        uint32_t familyIdxGraphics;
        uint32_t familyIdxTransfer;
        uint32_t familyIdxCompute;
        uint32_t familyIdxPresent;
        RQueue queueGraphics;
        RQueue queueTransfer;
        RQueue queueCompute;
        RQueue queuePresent;
    } vk;
};

void vk_create_device(struct RDeviceObj* obj, const RDeviceInfo& info);
void vk_destroy_device(struct RDeviceObj* obj);

} // namespace LD
