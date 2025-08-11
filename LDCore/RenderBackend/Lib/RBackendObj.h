#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vk_mem_alloc.h>    // hide
#include <vulkan/vulkan.hpp> // hide

#define PIPELINE_LAYOUT_MAX_RESOURCE_SETS 4

namespace LD {

struct RObjectID
{
    // NOTE: not atomic, currently only main thread
    //       may create and destroy objects
    static uint64_t sCounter;

    inline static uint64_t get() { return sCounter++; }
};

/// @brief Vulkan physical device properties
struct PhysicalDevice
{
    VkPhysicalDevice handle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProps;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VkSampleCountFlags msaaCount;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkFormat> depthStencilFormats; /// formats with VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    std::vector<VkQueueFamilyProperties> familyProps;
    std::vector<VkPresentModeKHR> presentModes;
};

/// @brief information to create a Vulkan Swapchain
struct SwapchainInfo
{
    VkFormat imageFormat;
    VkPresentModeKHR presentMode;
    VkColorSpaceKHR imageColorSpace;
    bool vsyncHint;
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
    uint64_t rid;
    RDevice device;
    RBufferInfo info;
    void* hostMap;

    void (*map)(RBufferObj* self);
    void* (*map_read)(RBufferObj* self, uint64_t offset, uint64_t size);
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
    uint64_t rid;
    RDevice device;
    RImageInfo info;
    std::unordered_set<uint32_t> fboHashes;

    struct
    {
        VmaAllocation vma;
        VkImage handle;
        VkImageView viewHandle;
        VkSampler samplerHandle;
    } vk;
};

/// @brief while RPassInfo contains transient pointer members,
///        this data representation is safe to be read at any time.
struct RPassInfoData
{
    RSampleCountBit samples;
    uint32_t colorAttachmentCount;
    std::vector<RPassColorAttachment> colorAttachments;
    std::vector<RPassResolveAttachment> colorResolveAttachments;
    std::optional<RPassDepthStencilAttachment> depthStencilAttachment;
    std::optional<RPassDependency> dependency;
};

struct RPassObj
{
    uint64_t rid;
    uint32_t hash;
    uint32_t colorAttachmentCount;
    RSampleCountBit samples;
    bool hasDepthStencilAttachment;

    struct
    {
        VkRenderPass handle;
    } vk;
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

    struct
    {
        VkFramebuffer handle;
    } vk;
};

struct RPipelineLayoutObj;

struct RCommandListObj
{
    uint64_t rid;
    RDeviceObj* deviceObj;
    RPassInfoData currentPass;

    void (*begin)(RCommandListObj* self, bool oneTimeSubmit);
    void (*end)(RCommandListObj* self);
    void (*cmd_begin_pass)(RCommandListObj* self, const RPassBeginInfo& passBI);
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
    void (*cmd_end_pass)(RCommandListObj* self);
    void (*cmd_buffer_memory_barrier)(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier);
    void (*cmd_image_memory_barrier)(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);
    void (*cmd_copy_buffer)(RCommandListObj* self, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);
    void (*cmd_copy_buffer_to_image)(RCommandListObj* self, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);
    void (*cmd_copy_image_to_buffer)(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions);
    void (*cmd_blit_image)(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter);

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
    uint64_t rid;
    std::vector<RCommandList> lists;
    RDeviceObj* deviceObj;

    RCommandList (*allocate)(RCommandPoolObj* self, RCommandListObj* listObj);
    void (*reset)(RCommandPoolObj* self);

    void init_vk_api();

    struct
    {
        VkDevice device;
        VkCommandPool handle;
    } vk;
};

struct RShaderObj
{
    uint64_t rid;
    RShaderType type;

    struct
    {
        VkShaderModule handle;
    } vk;
};

struct RSetLayoutObj
{
    uint64_t rid;
    uint32_t hash;
    RDeviceObj* deviceObj;

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
    uint64_t rid;
    LinearAllocator setLA;
    RDeviceObj* deviceObj;
    RSetLayoutObj* layoutObj;

    RSet (*allocate)(RSetPoolObj* self, RSetObj* setObj);
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
    uint64_t rid;
    uint32_t hash;
    uint32_t setCount;
    RSetLayoutObj* setLayoutObjs[PIPELINE_LAYOUT_MAX_RESOURCE_SETS];

    struct
    {
        VkPipelineLayout handle;
    } vk;
};

struct RPipelineObj
{
    uint64_t rid;
    RDeviceObj* deviceObj;
    RPipelineLayoutObj* layoutObj;

    struct
    {
        bool depthTestEnabled;
        RPassObj* passObj;
        std::vector<RColorComponentFlags> colorWriteMasks;
    } variant;

    void (*create_variant)(RPipelineObj* self);

    void init_vk_api();

    struct
    {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCI;
        std::vector<VkVertexInputAttributeDescription> attributeD;
        std::vector<VkVertexInputBindingDescription> bindingD;
        std::vector<VkPipelineColorBlendAttachmentState> blendStates;
        std::unordered_map<uint32_t, VkPipeline> handles;
        VkPipelineViewportStateCreateInfo viewportSCI;
        VkPipelineVertexInputStateCreateInfo vertexInputSCI;
        VkPipelineInputAssemblyStateCreateInfo inputAsmSCI;
        VkPipelineTessellationStateCreateInfo tessellationSCI;
        VkPipelineRasterizationStateCreateInfo rasterizationSCI;
        VkPipelineDepthStencilStateCreateInfo depthStencilSCI;
        VkPipelineColorBlendStateCreateInfo colorBlendSCI;
        uint32_t variantHash;
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
    uint64_t rid;

    struct
    {
        VkSemaphore handle;
    } vk;
};

struct RFenceObj
{
    uint64_t rid;

    struct
    {
        VkFence handle;
    } vk;
};

struct RDeviceObj
{
    uint64_t rid;
    uint32_t frameIndex;
    RDeviceBackend backend;
    bool isHeadless;

    RSemaphore (*create_semaphore)(RDeviceObj* self, RSemaphoreObj* semaphoreObj);
    void (*destroy_semaphore)(RDeviceObj* self, RSemaphore semaphore);

    RFence (*create_fence)(RDeviceObj* self, bool createSignaled, RFenceObj* fenceObj);
    void (*destroy_fence)(RDeviceObj* self, RFence fence);

    RBuffer (*create_buffer)(RDeviceObj* self, const RBufferInfo& bufferI, RBufferObj* bufferObj);
    void (*destroy_buffer)(RDeviceObj* self, RBuffer buffer);

    RImage (*create_image)(RDeviceObj* self, const RImageInfo& imageI, RImageObj* imageObj);
    void (*destroy_image)(RDeviceObj* self, RImage image);

    void (*create_pass)(RDeviceObj* self, const RPassInfo& passI, RPassObj* passObj);
    void (*destroy_pass)(RDeviceObj* self, RPassObj* passObj);

    void (*create_framebuffer)(RDeviceObj* self, const RFramebufferInfo& fbI, RFramebufferObj* framebufferObj);
    void (*destroy_framebuffer)(RDeviceObj* self, RFramebufferObj* framebufferObj);

    RCommandPool (*create_command_pool)(RDeviceObj* self, const RCommandPoolInfo& poolI, RCommandPoolObj* poolObj);
    void (*destroy_command_pool)(RDeviceObj* self, RCommandPool pool);

    RShader (*create_shader)(RDeviceObj* self, const RShaderInfo& shaderI, RShaderObj* shaderObj);
    void (*destroy_shader)(RDeviceObj* self, RShader shader);

    RSetPool (*create_set_pool)(RDeviceObj* self, const RSetPoolInfo& poolI, RSetPoolObj* poolObj);
    void (*destroy_set_pool)(RDeviceObj* self, RSetPool pool);

    void (*create_set_layout)(RDeviceObj* self, const RSetLayoutInfo& layoutI, RSetLayoutObj* layoutObj);
    void (*destroy_set_layout)(RDeviceObj* self, RSetLayoutObj* layout);

    void (*create_pipeline_layout)(RDeviceObj* self, const RPipelineLayoutInfo& layoutI, RPipelineLayoutObj* layoutObj);
    void (*destroy_pipeline_layout)(RDeviceObj* self, RPipelineLayoutObj* layoutObj);

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

    void init_vk_api();

    RPassObj* get_or_create_pass_obj(const RPassInfo& passI);
    RSetLayoutObj* get_or_create_set_layout_obj(const RSetLayoutInfo& layoutI);
    RPipelineLayoutObj* get_or_create_pipeline_layout_obj(const RPipelineLayoutInfo& layoutI);
    RFramebufferObj* get_or_create_framebuffer_obj(const RFramebufferInfo& framebufferI);

    struct
    {
        VmaAllocator vma;
        VkInstance instance;
        VkSurfaceKHR surface;
        PhysicalDevice pdevice;
        Swapchain swapchain;
        VkDevice device;
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
