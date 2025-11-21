#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Memory.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // hide from user
#define VMA_VULKAN_VERSION 1003000
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h> // hide from user
#include <vulkan/vulkan.hpp>

#include "RBackendObj.h"
#include "RShaderCompiler.h"
#include "RUtilCommon.h"
#include "RUtilVK.h"

// RBackendVK.cpp
// - Vulkan 1.3 backend implementation

#define VK_CHECK(CALL)                                                      \
    do                                                                      \
    {                                                                       \
        VkResult result_ = CALL;                                            \
        if (result_ != VK_SUCCESS)                                          \
        {                                                                   \
            sLog.error("{}:{} VK_CHECK failed with VkResult {}",            \
                       std::filesystem::path(__FILE__).filename().string(), \
                       __LINE__,                                            \
                       RUtil::get_vk_result_cstr(result_));                 \
            LD_DEBUG_BREAK;                                                 \
        }                                                                   \
    } while (0)

// clang-format off
#define APPLICATION_NAME      "LudensVulkanBackend"
#define APPLICATION_VERSION   VK_MAKE_API_VERSION(0, 0, 0, 0)
#define API_VERSION           VK_API_VERSION_1_3
// clang-format on

#define FRAMES_IN_FLIGHT 2

namespace LD {

static Log sLog("RBackendVK");

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

/// @brief Vulkan extension function pointers
struct VulkanExtensions
{
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
} sExt;

/// @brief if the instance extension VK_EXT_debug_utils is supported,
///        attach our debug messenger callbacks during debug build.
class VulkanDebugMessenger
{
public:
    VulkanDebugMessenger() = delete;
    VulkanDebugMessenger(VkInstance instance, VkResult* result);
    ~VulkanDebugMessenger();

private:
    VkDebugUtilsMessengerEXT mHandle;
    VkInstance mInstance;

    static VKAPI_ATTR VkBool32 callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

VulkanDebugMessenger::VulkanDebugMessenger(VkInstance instance, VkResult* result)
    : mInstance(instance)
{
    VkDebugUtilsMessengerCreateInfoEXT messengerCI = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &VulkanDebugMessenger::callback,
        .pUserData = this,
    };

    *result = sExt.vkCreateDebugUtilsMessengerEXT(instance, &messengerCI, nullptr, &mHandle);

    if (*result != VK_SUCCESS)
    {
        sLog.error("vkCreateDebugUtilsMessengerEXT failed: {}", (int)*result);
        return;
    }
}

VulkanDebugMessenger ::~VulkanDebugMessenger()
{
    if (mInstance != VK_NULL_HANDLE && mHandle != VK_NULL_HANDLE)
        sExt.vkDestroyDebugUtilsMessengerEXT(mInstance, mHandle, nullptr);
}

VKAPI_ATTR VkBool32 VulkanDebugMessenger::callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        sLog.warn("vulkan validation layer:\n{}", pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        sLog.error("vulkan validation layer:\n{}", pCallbackData->pMessage);
        LD_DEBUG_BREAK;
    }

    return VK_FALSE;
}

VulkanDebugMessenger* sDebugMessenger = nullptr;

static size_t vk_device_get_obj_size(RType objType);

static void vk_device_semaphore_ctor(RSemaphoreObj* obj);
static void vk_device_semaphore_dtor(RSemaphoreObj* obj);
static RSemaphore vk_device_create_semaphore(RDeviceObj* self, RSemaphoreObj* obj);
static void vk_device_destroy_semaphore(RDeviceObj* self, RSemaphore semaphore);

static void vk_device_fence_ctor(RFenceObj* baseObj);
static void vk_device_fence_dtor(RFenceObj* baseObj);
static RFence vk_device_create_fence(RDeviceObj* self, bool createSignaled, RFenceObj* obj);
static void vk_device_destroy_fence(RDeviceObj* self, RFence fence);

static void vk_device_buffer_ctor(RBufferObj* baseObj);
static void vk_device_buffer_dtor(RBufferObj* baseObj);
static RBuffer vk_device_create_buffer(RDeviceObj* self, const RBufferInfo& bufferI, RBufferObj* obj);
static void vk_device_destroy_buffer(RDeviceObj* self, RBuffer buffer);

static void vk_device_image_ctor(RImageObj* baseObj);
static void vk_device_image_dtor(RImageObj* baseObj);
static RImage vk_device_create_image(RDeviceObj* self, const RImageInfo& imageI, RImageObj* obj);
static void vk_device_destroy_image(RDeviceObj* self, RImage image);

static void vk_device_pass_ctor(RPassObj* baseObj);
static void vk_device_pass_dtor(RPassObj* baseObj);
static void vk_device_create_pass(RDeviceObj* self, const RPassInfo& passI, RPassObj* obj);
static void vk_device_destroy_pass(RDeviceObj* self, RPassObj* obj);

static void vk_device_framebuffer_ctor(RFramebufferObj* baseObj);
static void vk_device_framebuffer_dtor(RFramebufferObj* baseObj);
static void vk_device_create_framebuffer(RDeviceObj* self, const RFramebufferInfo& fbI, RFramebufferObj* obj);
static void vk_device_destroy_framebuffer(RDeviceObj* self, RFramebufferObj* obj);

static void vk_device_command_pool_ctor(RCommandPoolObj* baseObj);
static void vk_device_command_pool_dtor(RCommandPoolObj* baseObj);
static RCommandPool vk_device_create_command_pool(RDeviceObj* self, const RCommandPoolInfo& poolI, RCommandPoolObj* obj);
static void vk_device_destroy_command_pool(RDeviceObj* self, RCommandPool pool);

static void vk_device_command_list_ctor(RCommandListObj* baseObj);
static void vk_device_command_list_dtor(RCommandListObj* baseObj);

static void vk_device_shader_ctor(RShaderObj* baseObj);
static void vk_device_shader_dtor(RShaderObj* baseObj);
static RShader vk_device_create_shader(RDeviceObj* self, const RShaderInfo& shaderI, RShaderObj* obj);
static void vk_device_destroy_shader(RDeviceObj* self, RShader shader);

static void vk_device_set_pool_ctor(RSetPoolObj* baseObj);
static void vk_device_set_pool_dtor(RSetPoolObj* baseObj);
static RSetPool vk_device_create_set_pool(RDeviceObj* self, const RSetPoolInfo& poolI, RSetPoolObj* obj);
static void vk_device_destroy_set_pool(RDeviceObj* self, RSetPool pool);

static void vk_device_set_ctor(RSetObj* baseObj);
static void vk_device_set_dtor(RSetObj* baseObj);

static void vk_device_set_layout_ctor(RSetLayoutObj* baseObj);
static void vk_device_set_layout_dtor(RSetLayoutObj* baseObj);
static void vk_device_create_set_layout(RDeviceObj* self, const RSetLayoutInfo& layoutI, RSetLayoutObj* obj);
static void vk_device_destroy_set_layout(RDeviceObj* self, RSetLayoutObj* layoutObj);

static void vk_device_pipeline_layout_ctor(RPipelineLayoutObj* baseObj);
static void vk_device_pipeline_layout_dtor(RPipelineLayoutObj* baseObj);
static void vk_device_create_pipeline_layout(RDeviceObj* self, const RPipelineLayoutInfo& layoutI, RPipelineLayoutObj* obj);
static void vk_device_destroy_pipeline_layout(RDeviceObj* self, RPipelineLayoutObj* layoutObj);

static void vk_device_pipeline_ctor(RPipelineObj* baseObj);
static void vk_device_pipeline_dtor(RPipelineObj* baseObj);
static RPipeline vk_device_create_pipeline(RDeviceObj* self, const RPipelineInfo& pipelineI, RPipelineObj* obj);
static RPipeline vk_device_create_compute_pipeline(RDeviceObj* self, const RComputePipelineInfo& pipelineI, RPipelineObj* pipelineObj);
static void vk_device_destroy_pipeline(RDeviceObj* self, RPipeline pipeline);
static void vk_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI);
static void vk_device_pipeline_variant_color_write_mask(RDeviceObj* self, RPipelineObj* pipelineObj, uint32_t index, RColorComponentFlags mask);
static void vk_device_pipeline_variant_depth_test_enable(RDeviceObj* self, RPipelineObj* pipelineObj, bool enable);

static void vk_device_update_set_images(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates);
static void vk_device_update_set_buffers(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates);

static uint32_t vk_device_next_frame(RDeviceObj* self, RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete);
static void vk_device_present_frame(RDeviceObj* self);
static void vk_device_get_depth_stencil_formats(RDeviceObj* self, RFormat* formats, uint32_t& count);
static RSampleCountBit vk_device_get_max_sample_count(RDeviceObj* self);
static RFormat vk_device_get_swapchain_color_format(RDeviceObj* self);
static RImage vk_device_get_swapchain_color_attachment(RDeviceObj* self, uint32_t imageIdx);
static uint32_t vk_device_get_swapchain_image_count(RDeviceObj* self);
static void vk_device_get_swapchain_extent(RDeviceObj* self, uint32_t* width, uint32_t* height);
static uint32_t vk_device_get_frames_in_flight_count(RDeviceObj* self);
static RQueue vk_device_get_graphics_queue(RDeviceObj* self);
static void vk_device_wait_idle(RDeviceObj* self);

static constexpr RDeviceAPI sRDeviceVKAPI = {
    .get_obj_size = &vk_device_get_obj_size,
    .semaphore_ctor = &vk_device_semaphore_ctor,
    .semaphore_dtor = &vk_device_semaphore_dtor,
    .create_semaphore = &vk_device_create_semaphore,
    .destroy_semaphore = &vk_device_destroy_semaphore,
    .fence_ctor = &vk_device_fence_ctor,
    .fence_dtor = &vk_device_fence_dtor,
    .create_fence = &vk_device_create_fence,
    .destroy_fence = &vk_device_destroy_fence,
    .buffer_ctor = &vk_device_buffer_ctor,
    .buffer_dtor = &vk_device_buffer_dtor,
    .create_buffer = &vk_device_create_buffer,
    .destroy_buffer = &vk_device_destroy_buffer,
    .image_ctor = &vk_device_image_ctor,
    .image_dtor = &vk_device_image_dtor,
    .create_image = &vk_device_create_image,
    .destroy_image = &vk_device_destroy_image,
    .pass_ctor = &vk_device_pass_ctor,
    .pass_dtor = &vk_device_pass_dtor,
    .create_pass = &vk_device_create_pass,
    .destroy_pass = &vk_device_destroy_pass,
    .framebuffer_ctor = &vk_device_framebuffer_ctor,
    .framebuffer_dtor = &vk_device_framebuffer_dtor,
    .create_framebuffer = &vk_device_create_framebuffer,
    .destroy_framebuffer = &vk_device_destroy_framebuffer,
    .command_pool_ctor = &vk_device_command_pool_ctor,
    .command_pool_dtor = &vk_device_command_pool_dtor,
    .create_command_pool = &vk_device_create_command_pool,
    .destroy_command_pool = &vk_device_destroy_command_pool,
    .command_list_ctor = &vk_device_command_list_ctor,
    .command_list_dtor = &vk_device_command_list_dtor,
    .shader_ctor = &vk_device_shader_ctor,
    .shader_dtor = &vk_device_shader_dtor,
    .create_shader = &vk_device_create_shader,
    .destroy_shader = &vk_device_destroy_shader,
    .set_pool_ctor = &vk_device_set_pool_ctor,
    .set_pool_dtor = &vk_device_set_pool_dtor,
    .create_set_pool = &vk_device_create_set_pool,
    .destroy_set_pool = &vk_device_destroy_set_pool,
    .set_ctor = &vk_device_set_ctor,
    .set_dtor = &vk_device_set_dtor,
    .set_layout_ctor = &vk_device_set_layout_ctor,
    .set_layout_dtor = &vk_device_set_layout_dtor,
    .create_set_layout = &vk_device_create_set_layout,
    .destroy_set_layout = &vk_device_destroy_set_layout,
    .pipeline_layout_ctor = &vk_device_pipeline_layout_ctor,
    .pipeline_layout_dtor = &vk_device_pipeline_layout_dtor,
    .create_pipeline_layout = &vk_device_create_pipeline_layout,
    .destroy_pipeline_layout = &vk_device_destroy_pipeline_layout,
    .pipeline_ctor = &vk_device_pipeline_ctor,
    .pipeline_dtor = &vk_device_pipeline_dtor,
    .create_pipeline = &vk_device_create_pipeline,
    .create_compute_pipeline = &vk_device_create_compute_pipeline,
    .destroy_pipeline = &vk_device_destroy_pipeline,
    .pipeline_variant_pass = &vk_device_pipeline_variant_pass,
    .pipeline_variant_color_write_mask = &vk_device_pipeline_variant_color_write_mask,
    .pipeline_variant_depth_test_enable = &vk_device_pipeline_variant_depth_test_enable,
    .update_set_images = &vk_device_update_set_images,
    .update_set_buffers = &vk_device_update_set_buffers,
    .next_frame = &vk_device_next_frame,
    .present_frame = &vk_device_present_frame,
    .get_depth_stencil_formats = &vk_device_get_depth_stencil_formats,
    .get_max_sample_count = &vk_device_get_max_sample_count,
    .get_swapchain_color_format = &vk_device_get_swapchain_color_format,
    .get_swapchain_color_attachment = &vk_device_get_swapchain_color_attachment,
    .get_swapchain_image_count = &vk_device_get_swapchain_image_count,
    .get_swapchain_extent = &vk_device_get_swapchain_extent,
    .get_frames_in_flight_count = &vk_device_get_frames_in_flight_count,
    .get_graphics_queue = &vk_device_get_graphics_queue,
    .wait_idle = &vk_device_wait_idle,
};

/// @brief Vulkan device object.
struct RDeviceVKObj : RDeviceObj
{
    RDeviceVKObj()
    {
        api = &sRDeviceVKAPI;
        backend = RDEVICE_BACKEND_VULKAN;
    }

    struct VK
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

static void vk_buffer_map(RBufferObj* self);
static void* vk_buffer_map_read(RBufferObj* baseSelf, uint64_t offset, uint64_t size);
static void vk_buffer_map_write(RBufferObj* self, uint64_t offset, uint64_t size, const void* data);
static void vk_buffer_unmap(RBufferObj* self);

static constexpr RBufferAPI sRBufferVKAPI = {
    .map = &vk_buffer_map,
    .map_read = &vk_buffer_map_read,
    .map_write = &vk_buffer_map_write,
    .unmap = &vk_buffer_unmap,
};

/// @brief Vulkan buffer object.
struct RBufferVKObj : RBufferObj
{
    RBufferVKObj()
    {
        api = &sRBufferVKAPI;
    }

    struct VK
    {
        VmaAllocation vma;
        VkBuffer handle;
    } vk;
};

/// @brief Vulkan image object.
struct RImageVKObj : RImageObj
{
    struct VK
    {
        VmaAllocation vma;
        VkImage handle;
        VkImageView viewHandle;
        VkSampler samplerHandle;
    } vk;
};

/// @brief Vulkan render pass object.
struct RPassVKObj : RPassObj
{
    struct
    {
        VkRenderPass handle;
    } vk;
};

/// @brief Vulkan framebuffer object.
struct RFramebufferVKObj : RFramebufferObj
{
    struct
    {
        VkFramebuffer handle;
    } vk;
};

static void vk_command_list_begin(RCommandListObj* self, bool oneTimeSubmit);
static void vk_command_list_end(RCommandListObj* self);
static void vk_command_list_reset(RCommandListObj* self);
static void vk_command_list_cmd_begin_pass(RCommandListObj* self, const RPassBeginInfo& passBI, RFramebufferObj* baseFBObj);
static void vk_command_list_cmd_push_constant(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t offset, uint32_t size, const void* data);
static void vk_command_list_cmd_bind_graphics_pipeline(RCommandListObj* self, RPipeline pipeline);
static void vk_command_list_cmd_bind_graphics_sets(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t setStart, uint32_t setCount, RSet* sets);
static void vk_command_list_cmd_bind_compute_pipeline(RCommandListObj* self, RPipeline pipeline);
static void vk_command_list_cmd_bind_compute_sets(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t setStart, uint32_t setCount, RSet* sets);
static void vk_command_list_cmd_bind_vertex_buffers(RCommandListObj* self, uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);
static void vk_command_list_cmd_bind_index_buffer(RCommandListObj* self, RBuffer buffer, RIndexType indexType);
static void vk_command_list_cmd_dispatch(RCommandListObj* self, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
static void vk_command_list_cmd_set_scissor(RCommandListObj* self, const Rect& scissor);
static void vk_command_list_cmd_draw(RCommandListObj* self, const RDrawInfo& drawI);
static void vk_command_list_cmd_draw_indexed(RCommandListObj* self, const RDrawIndexedInfo& drawI);
static void vk_command_list_cmd_draw_indirect(RCommandListObj* self, const RDrawIndirectInfo& drawI);
static void vk_command_list_cmd_draw_indexed_indirect(RCommandListObj* self, const RDrawIndexedIndirectInfo& drawI);
static void vk_command_list_cmd_end_pass(RCommandListObj* self);
static void vk_command_list_cmd_buffer_memory_barrier(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier);
static void vk_command_list_cmd_image_memory_barrier(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);
static void vk_command_list_cmd_copy_buffer(RCommandListObj* self, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);
static void vk_command_list_cmd_copy_buffer_to_image(RCommandListObj* self, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);
static void vk_command_list_cmd_copy_image_to_buffer(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions);
static void vk_command_list_cmd_blit_image(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter);

static const RCommandListAPI sRCommandListVKAPI = {
    .begin = &vk_command_list_begin,
    .end = &vk_command_list_end,
    .reset = &vk_command_list_reset,
    .cmd_begin_pass = &vk_command_list_cmd_begin_pass,
    .cmd_push_constant = &vk_command_list_cmd_push_constant,
    .cmd_bind_graphics_pipeline = &vk_command_list_cmd_bind_graphics_pipeline,
    .cmd_bind_graphics_sets = &vk_command_list_cmd_bind_graphics_sets,
    .cmd_bind_compute_pipeline = &vk_command_list_cmd_bind_compute_pipeline,
    .cmd_bind_compute_sets = &vk_command_list_cmd_bind_compute_sets,
    .cmd_bind_vertex_buffers = &vk_command_list_cmd_bind_vertex_buffers,
    .cmd_bind_index_buffer = &vk_command_list_cmd_bind_index_buffer,
    .cmd_dispatch = &vk_command_list_cmd_dispatch,
    .cmd_set_scissor = &vk_command_list_cmd_set_scissor,
    .cmd_draw = &vk_command_list_cmd_draw,
    .cmd_draw_indexed = &vk_command_list_cmd_draw_indexed,
    .cmd_draw_indirect = &vk_command_list_cmd_draw_indirect,
    .cmd_draw_indexed_indirect = &vk_command_list_cmd_draw_indexed_indirect,
    .cmd_end_pass = &vk_command_list_cmd_end_pass,
    .cmd_buffer_memory_barrier = &vk_command_list_cmd_buffer_memory_barrier,
    .cmd_image_memory_barrier = &vk_command_list_cmd_image_memory_barrier,
    .cmd_copy_buffer = &vk_command_list_cmd_copy_buffer,
    .cmd_copy_buffer_to_image = &vk_command_list_cmd_copy_buffer_to_image,
    .cmd_copy_image_to_buffer = &vk_command_list_cmd_copy_image_to_buffer,
    .cmd_blit_image = &vk_command_list_cmd_blit_image,
};

/// @brief Vulkan command list object.
struct RCommandListVKObj : RCommandListObj
{
    RCommandListVKObj()
    {
        api = &sRCommandListVKAPI;
    }

    struct
    {
        VkDevice device;
        VkCommandBuffer handle;
    } vk;
};

static RCommandList vk_command_pool_allocate(RCommandPoolObj* self, RCommandListObj* listObj);
static void vk_command_pool_reset(RCommandPoolObj* self);

static const RCommandPoolAPI sRCommandPoolVKAPI = {
    .allocate = &vk_command_pool_allocate,
    .reset = &vk_command_pool_reset,
};

/// @brief Vulkan command pool object.
struct RCommandPoolVKObj : RCommandPoolObj
{
    RCommandPoolVKObj()
    {
        api = &sRCommandPoolVKAPI;
    }

    struct
    {
        VkDevice device;
        VkCommandPool handle;
    } vk;
};

/// @brief Vulkan shader object.
struct RShaderVKObj : RShaderObj
{
    struct
    {
        VkShaderModule handle;
    } vk;
};

/// @brief Vulkan set layout object.
struct RSetLayoutVKObj : RSetLayoutObj
{
    struct
    {
        VkDescriptorSetLayout handle;
    } vk;
};

/// @brief Vulkan set object.
struct RSetVKObj : RSetObj
{
    struct
    {
        VkDescriptorSet handle;
    } vk;
};

static RSet vk_set_pool_allocate(RSetPoolObj* self, RSetObj* setObj);
static void vk_set_pool_reset(RSetPoolObj* self);

static const RSetPoolAPI sRSetPoolVKAPI = {
    .allocate = &vk_set_pool_allocate,
    .reset = &vk_set_pool_reset,
};

/// @brief Vulkan set pool object.
struct RSetPoolVKObj : RSetPoolObj
{
    RSetPoolVKObj()
    {
        api = &sRSetPoolVKAPI;
    }

    struct
    {
        VkDevice device;
        VkDescriptorPool handle;
    } vk;
};

/// @brief Vulkan pipeline layout object.
struct RPipelineLayoutVKObj : RPipelineLayoutObj
{
    struct
    {
        VkPipelineLayout handle;
    } vk;
};

static void vk_pipeline_create_variant(RPipelineObj* self);

static const RPipelineAPI sPipelineVKAPI = {
    .create_variant = &vk_pipeline_create_variant,
};

/// @brief Vulkan pipeline object.
struct RPipelineVKObj : RPipelineObj
{
    RPipelineVKObj()
    {
        api = &sPipelineVKAPI;
    }

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

static void vk_queue_wait_idle(RQueueObj* self);
static void vk_queue_submit(RQueueObj* self, const RSubmitInfo& submitI, RFence fence);

static const RQueueAPI sRQueueVKAPI = {
    .wait_idle = &vk_queue_wait_idle,
    .submit = &vk_queue_submit,
};

/// @brief Vulkan queue object.
struct RQueueVKObj : RQueueObj
{
    RQueueVKObj()
    {
        api = &sRQueueVKAPI;
    }

    struct
    {
        uint32_t familyIdx;
        VkQueue handle;
    } vk;
};

/// @brief Vulkan semaphore object.
struct RSemaphoreVKObj : RSemaphoreObj
{
    struct
    {
        VkSemaphore handle;
    } vk;
};

/// @brief Vulkan fence object.
struct RFenceVKObj : RFenceObj
{
    struct
    {
        VkFence handle;
    } vk;
};

// @brief Vulkan frame boundary
struct VulkanFrame
{
    RFence frameComplete;
    RSemaphore imageAcquired;
    RSemaphore presentReady;
    RFenceVKObj frameCompleteObj;
    RSemaphoreVKObj imageAcquiredObj;
    RSemaphoreVKObj presentReadyObj;
} sVulkanFrames[FRAMES_IN_FLIGHT];

static VkResult acquire_next_image(RDeviceVKObj* obj, VkSemaphore imageAcquiredSemaphore);
static void choose_physical_device(RDeviceVKObj* obj);
static void configure_swapchain(RDeviceVKObj* obj, SwapchainInfo* swapchainI);
static void create_swapchain(RDeviceVKObj* obj, const SwapchainInfo& swapchainI);
static RImage create_swapchain_color_attachment(RDeviceVKObj* deviceObj, VkImage image, VkFormat colorFormat, uint32_t width, uint32_t height);
static void destroy_swapchain(RDeviceVKObj* obj);
static void destroy_swapchain_color_attachment(RDeviceVKObj* deviceObj, RImage attachment);
static void invalidate_swapchain(RDeviceVKObj* obj);
static void create_vma_allocator(RDeviceVKObj* obj);
static void destroy_vma_allocator(RDeviceVKObj* obj);
static RQueue create_queue(uint32_t queueFamilyIdx, VkQueue handle);
static void destroy_queue(RQueue queue);

// clang-format off
struct RTypeVK
{
    RType type;
    size_t byteSize;
} sTypeVKTable[] = {
    { RTYPE_DEVICE,          sizeof(RDeviceVKObj) },
    { RTYPE_SEMAPHORE,       sizeof(RSemaphoreVKObj)},
    { RTYPE_FENCE,           sizeof(RFenceVKObj)},
    { RTYPE_BUFFER,          sizeof(RBufferVKObj)},
    { RTYPE_IMAGE,           sizeof(RImageVKObj)},
    { RTYPE_SHADER,          sizeof(RShaderVKObj)},
    { RTYPE_SET_LAYOUT,      sizeof(RSetLayoutVKObj)},
    { RTYPE_SET,             sizeof(RSetVKObj)},
    { RTYPE_SET_POOL,        sizeof(RSetPoolVKObj)},
    { RTYPE_PASS,            sizeof(RPassVKObj)},
    { RTYPE_FRAMEBUFFER,     sizeof(RFramebufferVKObj)},
    { RTYPE_PIPELINE_LAYOUT, sizeof(RPipelineLayoutVKObj)},
    { RTYPE_PIPELINE,        sizeof(RPipelineVKObj)},
    { RTYPE_COMMAND_LIST,    sizeof(RCommandListVKObj)},
    { RTYPE_COMMAND_POOL,    sizeof(RCommandPoolVKObj)},
    { RTYPE_QUEUE,           sizeof(RQueueVKObj)},
};
// clang-format on

static_assert(sizeof(sTypeVKTable) / sizeof(*sTypeVKTable) == (size_t)RTYPE_ENUM_COUNT);

// RDrawInfo should already be eligible as indirect draw command struct
static_assert(sizeof(VkDrawIndirectCommand) == sizeof(RDrawInfo));
static_assert(offsetof(VkDrawIndirectCommand, vertexCount) == offsetof(RDrawInfo, vertexCount));
static_assert(offsetof(VkDrawIndirectCommand, instanceCount) == offsetof(RDrawInfo, instanceCount));
static_assert(offsetof(VkDrawIndirectCommand, firstVertex) == offsetof(RDrawInfo, vertexStart));
static_assert(offsetof(VkDrawIndirectCommand, firstInstance) == offsetof(RDrawInfo, instanceStart));

// RDrawIndexedInfo should already be eligible as indexed indirect draw command struct
static_assert(sizeof(VkDrawIndexedIndirectCommand) == sizeof(RDrawIndexedInfo));
static_assert(offsetof(VkDrawIndexedIndirectCommand, indexCount) == offsetof(RDrawIndexedInfo, indexCount));
static_assert(offsetof(VkDrawIndexedIndirectCommand, instanceCount) == offsetof(RDrawIndexedInfo, instanceCount));
static_assert(offsetof(VkDrawIndexedIndirectCommand, firstIndex) == offsetof(RDrawIndexedInfo, indexStart));
static_assert(offsetof(VkDrawIndexedIndirectCommand, vertexOffset) == offsetof(RDrawIndexedInfo, vertexOffset));
static_assert(offsetof(VkDrawIndexedIndirectCommand, firstInstance) == offsetof(RDrawIndexedInfo, instanceStart));

size_t vk_device_byte_size()
{
    return sizeof(RDeviceVKObj);
}

void vk_device_ctor(RDeviceObj* baseObj)
{
    auto* obj = (RDeviceVKObj*)baseObj;

    new (obj) RDeviceVKObj();
}

void vk_device_dtor(RDeviceObj* baseObj)
{
    auto* obj = (RDeviceVKObj*)baseObj;

    obj->~RDeviceVKObj();
}

void vk_create_device(RDeviceObj* baseSelf, const RDeviceInfo& deviceI)
{
    LD_PROFILE_SCOPE;

    auto* self = (RDeviceVKObj*)baseSelf;
    self->vk.surface = VK_NULL_HANDLE;
    self->glfw = deviceI.window;

    // get supported instance extensions
    uint32_t extCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> supportedInstanceExts(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, supportedInstanceExts.data());
    std::set<std::string> supportedInstanceExtSet;
    for (const VkExtensionProperties& extProperty : supportedInstanceExts)
        supportedInstanceExtSet.insert(std::string(extProperty.extensionName));

    // get supported instance layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> supportedInstanceLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, supportedInstanceLayers.data());
    std::set<std::string> supportedInstanceLayerSet;
    for (const VkLayerProperties& layerProperty : supportedInstanceLayers)
        supportedInstanceLayerSet.insert(std::string(layerProperty.layerName));

    std::set<std::string> desiredInstanceExtSet = {
#if defined(VK_EXT_debug_utils) && !defined(NDEBUG)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    if (!self->isHeadless)
    {
        // NOTE: make sure glfwInit() is called before this
        LD_ASSERT(glfwVulkanSupported() == GLFW_TRUE);

        // already contains VK_KHR_surface
        uint32_t glfwExtCount;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
        for (uint32_t i = 0; i < glfwExtCount; i++)
            desiredInstanceExtSet.insert(glfwExts[i]);
    }

    // SPACE: insert any other user-requested extensions into set

    // requested extensions = desired && supported extensions
    std::vector<const char*> requestedInstanceExts;
    for (const std::string& desiredExt : desiredInstanceExtSet)
    {
        if (supportedInstanceExtSet.contains(desiredExt))
            requestedInstanceExts.push_back(desiredExt.c_str());
    }

    std::set<std::string> desiredInstanceLayerSet = {
        "VK_LAYER_KHRONOS_validation",
    };

    // requested layers = desired && supported layers
    std::vector<const char*> requestedInstanceLayers;
    for (const std::string& desiredLayer : desiredInstanceLayerSet)
    {
        if (supportedInstanceLayerSet.contains(desiredLayer))
            requestedInstanceLayers.push_back(desiredLayer.c_str());
    }

    VkApplicationInfo appI{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = APPLICATION_NAME,
        .applicationVersion = APPLICATION_VERSION,
        .apiVersion = API_VERSION,
    };

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appI,
#ifndef NDEBUG
        .enabledLayerCount = (uint32_t)requestedInstanceLayers.size(),
#else
        .enabledLayerCount = 0,
#endif
        .ppEnabledLayerNames = requestedInstanceLayers.data(),
        .enabledExtensionCount = (uint32_t)requestedInstanceExts.size(),
        .ppEnabledExtensionNames = requestedInstanceExts.data(),
    };

    VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &self->vk.instance));
    sExt.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(self->vk.instance, "vkCreateDebugUtilsMessengerEXT");
    sExt.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(self->vk.instance, "vkDestroyDebugUtilsMessengerEXT");

#ifndef NDEBUG
    VkResult result;
    sDebugMessenger = heap_new<VulkanDebugMessenger>(MEMORY_USAGE_RENDER, self->vk.instance, &result);
    if (result != VK_SUCCESS)
        heap_delete<VulkanDebugMessenger>(sDebugMessenger);
#endif

    if (!self->isHeadless)
    {
        // delegate surface creation to GLFW
        VK_CHECK(glfwCreateWindowSurface(self->vk.instance, self->glfw, nullptr, &self->vk.surface));
    }

    // choose a physical device, taking surface capabilities into account
    choose_physical_device(self);
    LD_ASSERT(self->vk.pdevice.handle != VK_NULL_HANDLE);

    // NOTE: here we are following the most basic use case of having one queue for each family
    uint32_t familyCount = (uint32_t)self->vk.pdevice.familyProps.size();
    uint32_t familyIdxGraphics = familyCount;
    uint32_t familyIdxTransfer = familyCount;
    uint32_t familyIdxCompute = familyCount;
    uint32_t familyIdxPresent = familyCount;
    std::vector<VkDeviceQueueCreateInfo> queueCI(familyCount);
    float priority = 1.0f;

    PhysicalDevice& pdevice = self->vk.pdevice;

    for (uint32_t idx = 0; idx < familyCount; idx++)
    {
        queueCI[idx].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCI[idx].queueCount = 1;
        queueCI[idx].queueFamilyIndex = idx;
        queueCI[idx].pQueuePriorities = &priority;

        if (familyIdxGraphics == familyCount && (pdevice.familyProps[idx].queueFlags | VK_QUEUE_GRAPHICS_BIT))
            familyIdxGraphics = idx;

        if (familyIdxTransfer == familyCount && (pdevice.familyProps[idx].queueFlags | VK_QUEUE_TRANSFER_BIT))
            familyIdxTransfer = idx;

        if (familyIdxCompute == familyCount && (pdevice.familyProps[idx].queueFlags | VK_QUEUE_COMPUTE_BIT))
            familyIdxCompute = idx;

        if (self->vk.surface != VK_NULL_HANDLE)
        {
            VkBool32 supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(pdevice.handle, idx, self->vk.surface, &supported);
            if (familyIdxPresent == familyCount && supported)
                familyIdxPresent = idx;
        }
    }

    LD_ASSERT(familyIdxGraphics != familyCount && "graphics queue family not found");
    LD_ASSERT(familyIdxTransfer != familyCount && "transfer queue family not found");
    LD_ASSERT(familyIdxCompute != familyCount && "compute queue family not found");
    LD_ASSERT(!(self->vk.surface && familyIdxPresent == familyCount) && "present queue family not found");

    std::string queueFlags;
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxGraphics].queueFlags, queueFlags);
    sLog.info("Vulkan graphics queue family index {}: ({})", familyIdxGraphics, queueFlags.c_str());
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxTransfer].queueFlags, queueFlags);
    sLog.info("Vulkan transfer queue family index {}: ({})", familyIdxTransfer, queueFlags.c_str());
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxCompute].queueFlags, queueFlags);
    sLog.info("Vulkan compute queue family index {}:  ({})", familyIdxCompute, queueFlags.c_str());

    if (familyIdxPresent != familyCount)
    {
        RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxPresent].queueFlags, queueFlags);
        sLog.info("Vulkan present queue family index {}:  ({})", familyIdxPresent, queueFlags.c_str());
    }

    // create a logical device and retrieve queue handles
    std::vector<const char*> desiredDeviceExts;

#ifdef VK_KHR_swapchain
    if (self->vk.surface)
        desiredDeviceExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#endif // VK_KHR_swapchain

    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = (uint32_t)queueCI.size(),
        .pQueueCreateInfos = queueCI.data(),
        .enabledExtensionCount = (uint32_t)desiredDeviceExts.size(),
        .ppEnabledExtensionNames = desiredDeviceExts.data(),
        .pEnabledFeatures = &pdevice.deviceFeatures,
    };
    VK_CHECK(vkCreateDevice(self->vk.pdevice.handle, &deviceCI, nullptr, &self->vk.device));

    self->vk.familyIdxGraphics = familyIdxGraphics;
    self->vk.familyIdxTransfer = familyIdxTransfer;
    self->vk.familyIdxPresent = familyIdxPresent;
    self->vk.familyIdxCompute = familyIdxCompute;

    VkQueue queueHandle;

    vkGetDeviceQueue(self->vk.device, familyIdxGraphics, 0, &queueHandle);
    self->vk.queueGraphics = create_queue(familyIdxGraphics, queueHandle);

    vkGetDeviceQueue(self->vk.device, familyIdxTransfer, 0, &queueHandle);
    self->vk.queueTransfer = create_queue(familyIdxTransfer, queueHandle);

    vkGetDeviceQueue(self->vk.device, familyIdxCompute, 0, &queueHandle);
    self->vk.queueCompute = create_queue(familyIdxCompute, queueHandle);

    if (familyIdxPresent != familyCount)
    {
        vkGetDeviceQueue(self->vk.device, familyIdxPresent, 0, &queueHandle);
        self->vk.queuePresent = create_queue(familyIdxPresent, queueHandle);
    }
    else
    {
        // headless rendering
        self->vk.queuePresent = {};
    }

    // delegate memory management to VMA
    create_vma_allocator(self);

    if (self->vk.surface != VK_NULL_HANDLE)
    {
        // create swapchain
        SwapchainInfo swapchainI{.vsyncHint = deviceI.vsync};
        configure_swapchain(self, &swapchainI);
        create_swapchain(self, swapchainI);

        // frames in flight synchronization
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            VulkanFrame* frame = sVulkanFrames + i;

            frame->presentReady = vk_device_create_semaphore(self, &frame->presentReadyObj);
            frame->imageAcquired = vk_device_create_semaphore(self, &frame->imageAcquiredObj);
            frame->frameComplete = vk_device_create_fence(self, true, &frame->frameCompleteObj);
            frame->presentReadyObj.rid = RObjectID::get();
            frame->imageAcquiredObj.rid = RObjectID::get();
            frame->frameCompleteObj.rid = RObjectID::get();
        }
    }
}

void vk_destroy_device(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    vkDeviceWaitIdle(self->vk.device);

    if (self->vk.surface != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            vk_device_destroy_fence(self, sVulkanFrames[i].frameComplete);
            vk_device_destroy_semaphore(self, sVulkanFrames[i].imageAcquired);
            vk_device_destroy_semaphore(self, sVulkanFrames[i].presentReady);
        }

        destroy_swapchain(self);

        vkDestroySurfaceKHR(self->vk.instance, self->vk.surface, nullptr);
    }

    // all VMA allocations should be freed by now
    destroy_vma_allocator(self);

    if (self->vk.queuePresent)
        destroy_queue(self->vk.queuePresent);

    destroy_queue(self->vk.queueCompute);
    destroy_queue(self->vk.queueTransfer);
    destroy_queue(self->vk.queueGraphics);

    vkDestroyDevice(self->vk.device, nullptr);

    if (sDebugMessenger)
        heap_delete<VulkanDebugMessenger>(sDebugMessenger);

    vkDestroyInstance(self->vk.instance, nullptr);
}

static size_t vk_device_get_obj_size(RType objType)
{
    return sTypeVKTable[(int)objType].byteSize;
}

static void vk_device_semaphore_ctor(RSemaphoreObj* baseObj)
{
    auto* obj = (RSemaphoreVKObj*)baseObj;

    new (obj) RSemaphoreVKObj();
}

static void vk_device_semaphore_dtor(RSemaphoreObj* baseObj)
{
    auto* obj = (RSemaphoreVKObj*)baseObj;

    obj->~RSemaphoreVKObj();
}

static RSemaphore vk_device_create_semaphore(RDeviceObj* baseSelf, RSemaphoreObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RSemaphoreVKObj*)baseObj;

    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
    };
    VK_CHECK(vkCreateSemaphore(self->vk.device, &semaphoreCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_semaphore(RDeviceObj* baseSelf, RSemaphore semaphore)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RSemaphoreVKObj*)semaphore.unwrap();

    vkDestroySemaphore(self->vk.device, obj->vk.handle, nullptr);
}

static void vk_device_fence_ctor(RFenceObj* baseObj)
{
    auto* obj = (RFenceVKObj*)baseObj;

    new (obj) RFenceVKObj();
}

static void vk_device_fence_dtor(RFenceObj* baseObj)
{
    auto* obj = (RFenceVKObj*)baseObj;

    obj->~RFenceVKObj();
}

static RFence vk_device_create_fence(RDeviceObj* baseSelf, bool createSignaled, RFenceObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RFenceVKObj*)baseObj;

    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlags)0,
    };
    VK_CHECK(vkCreateFence(self->vk.device, &fenceCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_fence(RDeviceObj* baseSelf, RFence fence)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RFenceVKObj*)fence.unwrap();

    vkDestroyFence(self->vk.device, obj->vk.handle, nullptr);
}

static void vk_device_buffer_ctor(RBufferObj* baseObj)
{
    auto* obj = (RBufferVKObj*)baseObj;

    new (obj) RBufferVKObj();
}

static void vk_device_buffer_dtor(RBufferObj* baseObj)
{
    auto* obj = (RBufferVKObj*)baseObj;

    obj->~RBufferVKObj();
}

static RBuffer vk_device_create_buffer(RDeviceObj* baseSelf, const RBufferInfo& bufferI, RBufferObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RBufferVKObj*)baseObj;

    VmaAllocationCreateFlags vmaFlags = 0;
    VkMemoryPropertyFlags vkProps = 0;
    VkBufferUsageFlags vkUsage = 0;
    RUtil::cast_buffer_usage_vk(bufferI.usage, vkUsage);

    if (bufferI.hostVisible)
    {
        vkProps |= (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = (VkDeviceSize)bufferI.size,
        .usage = vkUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO:
        .queueFamilyIndexCount = 0,
    };
    VmaAllocationCreateInfo allocationCI{
        .flags = vmaFlags,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = vkProps,
    };

    VK_CHECK(vmaCreateBuffer(self->vk.vma, &bufferCI, &allocationCI, &obj->vk.handle, &obj->vk.vma, nullptr));

    return {obj};
}

static void vk_device_destroy_buffer(RDeviceObj* baseSelf, RBuffer buffer)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RBufferVKObj*)buffer.unwrap();

    vmaDestroyBuffer(self->vk.vma, obj->vk.handle, obj->vk.vma);
}

static void vk_device_image_ctor(RImageObj* baseObj)
{
    auto* obj = (RImageVKObj*)baseObj;

    new (obj) RImageVKObj();
}

static void vk_device_image_dtor(RImageObj* baseObj)
{
    auto* obj = (RImageVKObj*)baseObj;

    obj->~RImageVKObj();
}

static RImage vk_device_create_image(RDeviceObj* baseSelf, const RImageInfo& imageI, RImageObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RImageVKObj*)baseObj;

    VkFormat vkFormat;
    VkImageType vkType;
    VkImageViewType vkViewType;
    VkImageUsageFlags vkUsage;
    VkImageAspectFlags vkAspect;
    VkSampleCountFlagBits vkSamples;
    RUtil::cast_format_vk(imageI.format, vkFormat);
    RUtil::cast_image_type_vk(imageI.type, vkType);
    RUtil::cast_image_view_type_vk(imageI.type, vkViewType);
    RUtil::cast_image_usage_vk(imageI.usage, vkUsage);
    RUtil::cast_format_image_aspect_vk(imageI.format, vkAspect);
    RUtil::cast_sample_count_vk(imageI.samples, vkSamples);

    VkImageCreateFlags imageFlags = 0;
    if (imageI.type == RIMAGE_TYPE_CUBE)
        imageFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImageCreateInfo imageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = imageFlags,
        .imageType = vkType,
        .format = vkFormat,
        .extent = {.width = imageI.width, .height = imageI.height, .depth = imageI.depth},
        .mipLevels = 1,
        .arrayLayers = imageI.layers,
        .samples = vkSamples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = vkUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO:
        .queueFamilyIndexCount = 0,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocationCI{
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VK_CHECK(vmaCreateImage(self->vk.vma, &imageCI, &allocationCI, &obj->vk.handle, &obj->vk.vma, nullptr));

    VkImageSubresourceRange viewRange{
        .aspectMask = vkAspect,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    VkImageViewCreateInfo viewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = obj->vk.handle,
        .viewType = vkViewType,
        .format = vkFormat,
        .subresourceRange = viewRange,
    };

    VK_CHECK(vkCreateImageView(self->vk.device, &viewCI, nullptr, &obj->vk.viewHandle));

    if (vkUsage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        VkFilter vkFilter;
        VkSamplerMipmapMode vkMipmapMode;
        VkSamplerAddressMode vkAddressMode;
        RUtil::cast_filter_vk(imageI.sampler.filter, vkFilter);
        RUtil::cast_filter_mipmap_mode_vk(imageI.sampler.mipmapFilter, vkMipmapMode);
        RUtil::cast_sampler_address_mode_vk(imageI.sampler.addressMode, vkAddressMode);

        VkSamplerCreateInfo samplerCI{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = vkFilter,
            .minFilter = vkFilter,
            .mipmapMode = vkMipmapMode,
            .addressModeU = vkAddressMode,
            .addressModeV = vkAddressMode,
            .addressModeW = vkAddressMode,
            .mipLodBias = 0.0f,
            .minLod = 0.0f,
            .maxLod = 1.0f,
        };

        VK_CHECK(vkCreateSampler(self->vk.device, &samplerCI, nullptr, &obj->vk.samplerHandle));
    }
    else
        obj->vk.samplerHandle = VK_NULL_HANDLE;

    return {obj};
}

static void vk_device_destroy_image(RDeviceObj* baseSelf, RImage image)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RImageVKObj*)image.unwrap();

    if (obj->vk.samplerHandle != VK_NULL_HANDLE)
        vkDestroySampler(self->vk.device, obj->vk.samplerHandle, nullptr);

    vkDestroyImageView(self->vk.device, obj->vk.viewHandle, nullptr);
    vmaDestroyImage(self->vk.vma, obj->vk.handle, obj->vk.vma);
}

static void vk_device_pass_ctor(RPassObj* baseObj)
{
    auto* obj = (RPassVKObj*)baseObj;

    new (obj) RPassVKObj();
}

static void vk_device_pass_dtor(RPassObj* baseObj)
{
    auto* obj = (RPassVKObj*)baseObj;

    obj->~RPassVKObj();
}

// NOTE: the RPass is simplified to contain only a single Vulkan subpass,
//       multiple subpasses may be useful for tiled renderers commonly
//       found in mobile devices, but we keep the render pass API simple for now.
static void vk_device_create_pass(RDeviceObj* baseSelf, const RPassInfo& passI, RPassObj* basePassObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RPassVKObj*)basePassObj;

    std::vector<VkAttachmentDescription> attachmentD(passI.colorAttachmentCount);
    std::vector<VkAttachmentReference> colorAttachmentRefs(passI.colorAttachmentCount);
    std::vector<VkAttachmentReference> colorResolveAttachmentRefs(passI.colorAttachmentCount);
    VkAttachmentReference depthStencilAttachmentRef;

    for (uint32_t i = 0; i < passI.colorAttachmentCount; i++)
    {
        VkImageLayout passLayout;
        RUtil::cast_image_layout_vk(passI.colorAttachments[i].passLayout, passLayout);
        RUtil::cast_pass_color_attachment_vk(passI.colorAttachments[i], passI.samples, attachmentD[i]);

        colorAttachmentRefs[i].attachment = i;
        colorAttachmentRefs[i].layout = passLayout;
    }

    if (passI.depthStencilAttachment)
    {
        attachmentD.resize(passI.colorAttachmentCount + 1);

        VkImageLayout passLayout;
        RUtil::cast_image_layout_vk(passI.depthStencilAttachment->passLayout, passLayout);
        RUtil::cast_pass_depth_stencil_attachment_vk(*passI.depthStencilAttachment, passI.samples, attachmentD.back());

        depthStencilAttachmentRef.attachment = (uint32_t)attachmentD.size() - 1;
        depthStencilAttachmentRef.layout = passLayout;
    }

    if (passI.colorResolveAttachments)
    {
        for (uint32_t i = 0; i < passI.colorAttachmentCount; i++)
        {
            VkAttachmentDescription description;
            RFormat colorFormat = passI.colorAttachments[i].colorFormat;
            VkImageLayout passLayout;

            RUtil::cast_image_layout_vk(passI.colorResolveAttachments[i].passLayout, passLayout);
            RUtil::cast_pass_color_resolve_attachment_vk(passI.colorResolveAttachments[i], colorFormat, description);
            attachmentD.push_back(description);

            colorResolveAttachmentRefs[i].attachment = (uint32_t)attachmentD.size() - 1;
            colorResolveAttachmentRefs[i].layout = passLayout;
        }
    }

    VkSubpassDescription subpassDesc{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .colorAttachmentCount = (uint32_t)colorAttachmentRefs.size(),
        .pColorAttachments = colorAttachmentRefs.data(),
        .pResolveAttachments = passI.colorResolveAttachments ? colorResolveAttachmentRefs.data() : nullptr,
        .pDepthStencilAttachment = passI.depthStencilAttachment ? &depthStencilAttachmentRef : nullptr,
        .preserveAttachmentCount = 0,
    };

    uint32_t dependencyCount = 0;
    VkSubpassDependency subpassDep;

    if (passI.dependency)
    {
        dependencyCount = 1;
        RUtil::cast_pass_dependency_vk(*passI.dependency, VK_SUBPASS_EXTERNAL, 0, subpassDep);
    }

    VkRenderPassCreateInfo renderPassCI{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (uint32_t)attachmentD.size(),
        .pAttachments = attachmentD.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
        .dependencyCount = dependencyCount,
        .pDependencies = &subpassDep,
    };

    VK_CHECK(vkCreateRenderPass(self->vk.device, &renderPassCI, nullptr, &obj->vk.handle));
}

static void vk_device_destroy_pass(RDeviceObj* baseSelf, RPassObj* basePassObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* passObj = (RPassVKObj*)basePassObj;

    vkDestroyRenderPass(self->vk.device, passObj->vk.handle, nullptr);
}

static void vk_device_framebuffer_ctor(RFramebufferObj* baseObj)
{
    auto* obj = (RFramebufferVKObj*)baseObj;

    new (obj) RFramebufferVKObj();
}

static void vk_device_framebuffer_dtor(RFramebufferObj* baseObj)
{
    auto* obj = (RFramebufferVKObj*)baseObj;

    obj->~RFramebufferVKObj();
}

static void vk_device_create_framebuffer(RDeviceObj* baseSelf, const RFramebufferInfo& fbI, RFramebufferObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RFramebufferVKObj*)baseObj;

    std::vector<VkImageView> attachments(fbI.colorAttachmentCount);
    for (uint32_t i = 0; i < fbI.colorAttachmentCount; i++)
    {
        RImageVKObj* imageObj = static_cast<RImageVKObj*>(fbI.colorAttachments[i].unwrap());
        attachments[i] = imageObj->vk.viewHandle;
    }

    if (fbI.depthStencilAttachment)
    {
        RImage image = fbI.depthStencilAttachment;
        RImageVKObj* imageObj = (RImageVKObj*)image.unwrap();
        attachments.push_back(imageObj->vk.viewHandle);
    }

    if (fbI.colorResolveAttachments)
    {
        for (uint32_t i = 0; i < fbI.colorAttachmentCount; i++)
        {
            RImageVKObj* imageObj = static_cast<RImageVKObj*>(fbI.colorResolveAttachments[i].unwrap());
            attachments.push_back(imageObj->vk.viewHandle);
        }
    }

    VkFramebufferCreateInfo fbCI{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = static_cast<RPassVKObj*>(obj->passObj)->vk.handle,
        .attachmentCount = (uint32_t)attachments.size(),
        .pAttachments = attachments.data(),
        .width = fbI.width,
        .height = fbI.height,
        .layers = 1,
    };
    VK_CHECK(vkCreateFramebuffer(self->vk.device, &fbCI, nullptr, &obj->vk.handle));
}

static void vk_device_destroy_framebuffer(RDeviceObj* baseSelf, RFramebufferObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RFramebufferVKObj*)baseObj;

    vkDestroyFramebuffer(self->vk.device, obj->vk.handle, nullptr);
}

static void vk_device_command_pool_ctor(RCommandPoolObj* baseObj)
{
    auto* obj = (RCommandPoolVKObj*)baseObj;

    new (obj) RCommandPoolVKObj();
}

static void vk_device_command_pool_dtor(RCommandPoolObj* baseObj)
{
    auto* obj = (RCommandPoolVKObj*)baseObj;

    obj->~RCommandPoolVKObj();
}

static RCommandPool vk_device_create_command_pool(RDeviceObj* baseSelf, const RCommandPoolInfo& poolI, RCommandPoolObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RCommandPoolVKObj*)baseObj;

    obj->vk.device = self->vk.device;

    VkCommandPoolCreateInfo poolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = self->vk.familyIdxGraphics, // TODO: parameterize against poolI.queueType
    };

    if (obj->hintTransient)
        poolCI.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    if (obj->listResettable)
        poolCI.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK(vkCreateCommandPool(self->vk.device, &poolCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_command_pool(RDeviceObj* baseSelf, RCommandPool pool)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* poolObj = (RCommandPoolVKObj*)pool.unwrap();

    vkDestroyCommandPool(self->vk.device, poolObj->vk.handle, nullptr);
}

static void vk_device_command_list_ctor(RCommandListObj* baseObj)
{
    auto* obj = (RCommandListVKObj*)baseObj;

    new (obj) RCommandListVKObj();
}

static void vk_device_command_list_dtor(RCommandListObj* baseObj)
{
    auto* obj = (RCommandListVKObj*)baseObj;

    obj->~RCommandListVKObj();
}

static void vk_device_shader_ctor(RShaderObj* baseObj)
{
    auto* obj = (RShaderVKObj*)baseObj;

    new (obj) RShaderVKObj();
}

static void vk_device_shader_dtor(RShaderObj* baseObj)
{
    auto* obj = (RShaderVKObj*)baseObj;

    obj->~RShaderVKObj();
}

static RShader vk_device_create_shader(RDeviceObj* baseSelf, const RShaderInfo& shaderI, RShaderObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RShaderVKObj*)baseObj;

    VkShaderModuleCreateInfo shaderCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = obj->spirv.size() * 4,
        .pCode = obj->spirv.data(),
    };

    VK_CHECK(vkCreateShaderModule(self->vk.device, &shaderCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_shader(RDeviceObj* baseSelf, RShader shader)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    RShaderVKObj* shaderObj = (RShaderVKObj*)shader.unwrap();

    vkDestroyShaderModule(self->vk.device, shaderObj->vk.handle, nullptr);
}

static void vk_device_set_pool_ctor(RSetPoolObj* baseObj)
{
    auto* obj = (RSetPoolVKObj*)baseObj;

    new (obj) RSetPoolVKObj();
}

static void vk_device_set_pool_dtor(RSetPoolObj* baseObj)
{
    auto* obj = (RSetPoolVKObj*)baseObj;

    obj->~RSetPoolVKObj();
}

static RSetPool vk_device_create_set_pool(RDeviceObj* baseSelf, const RSetPoolInfo& poolI, RSetPoolObj* basePoolObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    RSetPoolVKObj* poolObj = (RSetPoolVKObj*)basePoolObj;
    poolObj->vk.device = self->vk.device;

    std::vector<VkDescriptorPoolSize> poolSizes(poolI.layout.bindingCount);

    for (uint32_t i = 0; i < poolI.layout.bindingCount; i++)
    {
        uint32_t arrayCount = std::max<uint32_t>(1, poolI.layout.bindings[i].arrayCount);

        VkDescriptorType vkType;
        RUtil::cast_binding_type_vk(poolI.layout.bindings[i].type, vkType);
        poolSizes[i].type = vkType;
        poolSizes[i].descriptorCount = arrayCount * poolI.maxSets;
    }

    VkDescriptorPoolCreateInfo poolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = poolI.maxSets,
        .poolSizeCount = (uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(self->vk.device, &poolCI, nullptr, &poolObj->vk.handle));

    return {poolObj};
}

static void vk_device_destroy_set_pool(RDeviceObj* baseSelf, RSetPool pool)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* poolObj = (RSetPoolVKObj*)pool.unwrap();

    vkDestroyDescriptorPool(self->vk.device, poolObj->vk.handle, nullptr);
}

static void vk_device_set_ctor(RSetObj* baseObj)
{
    auto* obj = (RSetVKObj*)baseObj;

    new (obj) RSetVKObj();
}

static void vk_device_set_dtor(RSetObj* baseObj)
{
    auto* obj = (RSetVKObj*)baseObj;

    obj->~RSetVKObj();
}

static void vk_device_set_layout_ctor(RSetLayoutObj* baseObj)
{
    auto* obj = (RSetLayoutVKObj*)baseObj;

    new (obj) RSetLayoutVKObj();
}

static void vk_device_set_layout_dtor(RSetLayoutObj* baseObj)
{
    auto* obj = (RSetLayoutVKObj*)baseObj;

    obj->~RSetLayoutVKObj();
}

static void vk_device_create_set_layout(RDeviceObj* baseSelf, const RSetLayoutInfo& layoutI, RSetLayoutObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RSetLayoutVKObj*)baseObj;

    std::vector<VkDescriptorSetLayoutBinding> bindings(layoutI.bindingCount);
    for (uint32_t i = 0; i < layoutI.bindingCount; i++)
        RUtil::cast_set_layout_binding_vk(layoutI.bindings[i], bindings[i]);

    VkDescriptorSetLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (uint32_t)bindings.size(),
        .pBindings = bindings.data(),
    };

    VK_CHECK(vkCreateDescriptorSetLayout(self->vk.device, &layoutCI, nullptr, &obj->vk.handle));
}

static void vk_device_destroy_set_layout(RDeviceObj* baseSelf, RSetLayoutObj* baseObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* obj = (RSetLayoutVKObj*)baseObj;

    vkDestroyDescriptorSetLayout(self->vk.device, obj->vk.handle, nullptr);
}

static void vk_device_pipeline_layout_ctor(RPipelineLayoutObj* baseObj)
{
    auto* obj = (RPipelineLayoutVKObj*)baseObj;

    new (obj) RPipelineLayoutVKObj();
}

static void vk_device_pipeline_layout_dtor(RPipelineLayoutObj* baseObj)
{
    auto* obj = (RPipelineLayoutVKObj*)baseObj;

    obj->~RPipelineLayoutVKObj();
}

static void vk_device_create_pipeline_layout(RDeviceObj* baseSelf, const RPipelineLayoutInfo& layoutI, RPipelineLayoutObj* baseLayoutObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* layoutObj = (RPipelineLayoutVKObj*)baseLayoutObj;

    // NOTE: Here we make the simplification that all pipelines use the minimum 128 bytes
    //       of push constant as a single range. Different pipelines will alias these bytes as
    //       different fields, but the pipeline layouts will be compatible as long as they have
    //       compatible set layouts, removing push constant compatability from the equation.
    VkPushConstantRange range{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = 128,
    };

    std::vector<VkDescriptorSetLayout> setLayoutHandles(layoutI.setLayoutCount);
    for (uint32_t i = 0; i < layoutObj->setCount; i++)
    {
        auto* setLayoutObj = static_cast<RSetLayoutVKObj*>(layoutObj->setLayoutObjs[i]);
        setLayoutHandles[i] = setLayoutObj->vk.handle;
    }

    VkPipelineLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t)setLayoutHandles.size(),
        .pSetLayouts = setLayoutHandles.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range,
    };

    VK_CHECK(vkCreatePipelineLayout(self->vk.device, &layoutCI, nullptr, &layoutObj->vk.handle));
}

static void vk_device_destroy_pipeline_layout(RDeviceObj* baseSelf, RPipelineLayoutObj* baseLayoutObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* layoutObj = (RPipelineLayoutVKObj*)baseLayoutObj;

    vkDestroyPipelineLayout(self->vk.device, layoutObj->vk.handle, nullptr);

    layoutObj->~RPipelineLayoutVKObj();
}

static void vk_device_pipeline_ctor(RPipelineObj* baseObj)
{
    auto* obj = (RPipelineVKObj*)baseObj;

    new (obj) RPipelineVKObj();
}

static void vk_device_pipeline_dtor(RPipelineObj* baseObj)
{
    auto* obj = (RPipelineVKObj*)baseObj;

    obj->~RPipelineVKObj();
}

static RPipeline vk_device_create_pipeline(RDeviceObj* baseSelf, const RPipelineInfo& pipelineI, RPipelineObj* basePipelineObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* pipelineObj = (RPipelineVKObj*)basePipelineObj;

    // NOTE: here we only initialize the base pipeline properties,
    //       the actual graphics pipeline is created when variant properties
    //       such as the render pass is known at a later stage.

    uint32_t swpWidth = self->vk.swapchain.width;
    uint32_t swpHeight = self->vk.swapchain.height;
    VkViewport viewport = RUtil::make_viewport(swpWidth, swpHeight);
    VkRect2D scissor = RUtil::make_scissor(swpWidth, swpHeight);

    pipelineObj->vk.viewportSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    pipelineObj->vk.shaderStageCI.resize(pipelineI.shaderCount);

    for (uint32_t i = 0; i < pipelineI.shaderCount; i++)
    {
        auto* shaderObj = static_cast<RShaderVKObj*>(pipelineI.shaders[i].unwrap());
        VkShaderStageFlagBits shaderStage;
        RUtil::cast_shader_type_vk(shaderObj->type, shaderStage);

        pipelineObj->vk.shaderStageCI[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineObj->vk.shaderStageCI[i].pNext = nullptr;
        pipelineObj->vk.shaderStageCI[i].pName = LD_GLSL_ENTRY_POINT;
        pipelineObj->vk.shaderStageCI[i].flags = 0;
        pipelineObj->vk.shaderStageCI[i].module = shaderObj->vk.handle;
        pipelineObj->vk.shaderStageCI[i].pSpecializationInfo = nullptr;
        pipelineObj->vk.shaderStageCI[i].stage = shaderStage;
    }

    pipelineObj->vk.attributeD.resize(pipelineI.vertexAttributeCount);
    for (uint32_t i = 0; i < pipelineI.vertexAttributeCount; i++)
        RUtil::cast_vertex_attribute_vk(pipelineI.vertexAttributes[i], i, pipelineObj->vk.attributeD[i]);

    pipelineObj->vk.bindingD.resize(pipelineI.vertexBindingCount);
    for (uint32_t i = 0; i < pipelineI.vertexBindingCount; i++)
        RUtil::cast_vertex_binding_vk(pipelineI.vertexBindings[i], i, pipelineObj->vk.bindingD[i]);

    pipelineObj->vk.vertexInputSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = (uint32_t)pipelineObj->vk.bindingD.size(),
        .pVertexBindingDescriptions = pipelineObj->vk.bindingD.data(),
        .vertexAttributeDescriptionCount = (uint32_t)pipelineObj->vk.attributeD.size(),
        .pVertexAttributeDescriptions = pipelineObj->vk.attributeD.data(),
    };

    VkPrimitiveTopology vkPrimitive;
    RUtil::cast_primitive_topology_vk(pipelineI.primitiveTopology, vkPrimitive);
    pipelineObj->vk.inputAsmSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = vkPrimitive,
        .primitiveRestartEnable = VK_FALSE,
    };

    pipelineObj->vk.tessellationSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0,
    };

    VkCullModeFlags vkCullMode;
    VkPolygonMode vkPolygonMode;
    RUtil::cast_cull_mode_vk(pipelineI.rasterization.cullMode, vkCullMode);
    RUtil::cast_polygon_mode_vk(pipelineI.rasterization.polygonMode, vkPolygonMode);
    pipelineObj->vk.rasterizationSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vkPolygonMode,
        .cullMode = vkCullMode,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = pipelineI.rasterization.lineWidth,
    };

    VkCompareOp vkDepthCompareOp;
    RUtil::cast_compare_op_vk(pipelineI.depthStencil.depthCompareOp, vkDepthCompareOp);
    pipelineObj->vk.depthStencilSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = pipelineI.depthStencil.depthTestEnabled,
        .depthWriteEnable = pipelineI.depthStencil.depthWriteEnabled,
        .depthCompareOp = vkDepthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE, // TODO:
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };
    pipelineObj->variant.depthTestEnabled = pipelineI.depthStencil.depthTestEnabled;

    uint32_t blendAttachmentCount = pipelineI.blend.colorAttachmentCount;
    const RPipelineBlendState* blendStates = pipelineI.blend.colorAttachments;
    pipelineObj->variant.colorWriteMasks.resize(blendAttachmentCount);
    pipelineObj->vk.blendStates.resize(blendAttachmentCount);
    for (uint32_t i = 0; i < blendAttachmentCount; i++)
    {
        VkPipelineColorBlendAttachmentState& vkBlendState = pipelineObj->vk.blendStates[i];

        vkBlendState.blendEnable = (VkBool32)blendStates[i].enabled;
        vkBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pipelineObj->variant.colorWriteMasks[i] = RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT | RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT;

        if (!vkBlendState.blendEnable)
            continue;

        RUtil::cast_blend_factor_vk(blendStates[i].srcColorFactor, vkBlendState.srcColorBlendFactor);
        RUtil::cast_blend_factor_vk(blendStates[i].dstColorFactor, vkBlendState.dstColorBlendFactor);
        RUtil::cast_blend_factor_vk(blendStates[i].srcAlphaFactor, vkBlendState.srcAlphaBlendFactor);
        RUtil::cast_blend_factor_vk(blendStates[i].dstAlphaFactor, vkBlendState.dstAlphaBlendFactor);
        RUtil::cast_blend_op_vk(blendStates[i].colorBlendOp, vkBlendState.colorBlendOp);
        RUtil::cast_blend_op_vk(blendStates[i].alphaBlendOp, vkBlendState.alphaBlendOp);
    }

    pipelineObj->vk.colorBlendSCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = (uint32_t)pipelineObj->vk.blendStates.size(),
        .pAttachments = pipelineObj->vk.blendStates.data(),
    };

    return {pipelineObj};
}

static RPipeline vk_device_create_compute_pipeline(RDeviceObj* baseSelf, const RComputePipelineInfo& pipelineI, RPipelineObj* basePipelineObj)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    const auto* shaderObj = static_cast<const RShaderVKObj*>(pipelineI.shader.unwrap());
    auto* layoutObj = (RPipelineLayoutVKObj*)self->get_or_create_pipeline_layout_obj(pipelineI.layout);
    auto* pipelineObj = (RPipelineVKObj*)basePipelineObj;

    VkPipelineShaderStageCreateInfo stage{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderObj->vk.handle,
        .pName = LD_GLSL_ENTRY_POINT,
        .pSpecializationInfo = nullptr,
    };

    VkComputePipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stage,
        .layout = layoutObj->vk.handle,
    };

    VkPipeline vkHandle;
    VK_CHECK(vkCreateComputePipelines(self->vk.device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &vkHandle));

    // compute pipelines currently have no variant properties
    pipelineObj->vk.handles[0] = vkHandle;

    return {pipelineObj};
}

static void vk_device_destroy_pipeline(RDeviceObj* baseSelf, RPipeline pipeline)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* pipelineObj = (RPipelineVKObj*)pipeline.unwrap();

    // destroy all variants
    for (auto& ite : pipelineObj->vk.handles)
        vkDestroyPipeline(self->vk.device, ite.second, nullptr);
}

static void vk_device_pipeline_variant_pass(RDeviceObj* baseSelf, RPipelineObj* pipelineObj, const RPassInfo& passI)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    pipelineObj->variant.passObj = self->get_or_create_pass_obj(passI);
}

static void vk_device_pipeline_variant_color_write_mask(RDeviceObj* baseSelf, RPipelineObj* pipelineObj, uint32_t index, RColorComponentFlags mask)
{
    LD_ASSERT((size_t)index < pipelineObj->variant.colorWriteMasks.size());

    pipelineObj->variant.colorWriteMasks[index] = mask;
}

void vk_device_pipeline_variant_depth_test_enable(RDeviceObj* baseSelf, RPipelineObj* pipelineObj, bool enable)
{
    // NOTE: the command list should call vkCmdSetDepthTestEnable when binding the graphics pipeline.
    //       Vulkan considers the depthTestEnabled dynamic state to be part of the command buffer
    //       instead of the graphics pipeline.

    pipelineObj->variant.depthTestEnabled = enable;
}

static void vk_device_update_set_images(RDeviceObj* baseSelf, uint32_t updateCount, const RSetImageUpdateInfo* updates)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    std::vector<VkDescriptorImageInfo> imageI;
    std::vector<VkWriteDescriptorSet> writes(updateCount);

    for (uint32_t i = 0; i < updateCount; i++)
    {
        const RSetImageUpdateInfo& update = updates[i];

        for (uint32_t j = 0; j < update.imageCount; j++)
        {
            RImageVKObj* imageObj = static_cast<RImageVKObj*>(update.images[j].unwrap());
            VkImageLayout vkLayout;
            RUtil::cast_image_layout_vk(update.imageLayouts[j], vkLayout);

            imageI.push_back({
                .sampler = imageObj->vk.samplerHandle,
                .imageView = imageObj->vk.viewHandle,
                .imageLayout = vkLayout,
            });
        }
    }

    uint32_t imageInfoBase = 0;

    for (uint32_t i = 0; i < updateCount; i++)
    {
        VkDescriptorType descriptorType;
        RUtil::cast_binding_type_vk(updates[i].imageBindingType, descriptorType);

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].pNext = nullptr;
        writes[i].dstSet = static_cast<const RSetVKObj*>(updates[i].set.unwrap())->vk.handle;
        writes[i].dstBinding = updates[i].dstBinding;
        writes[i].dstArrayElement = updates[i].dstArrayIndex;
        writes[i].descriptorCount = updates[i].imageCount;
        writes[i].descriptorType = descriptorType;
        writes[i].pImageInfo = imageI.data() + imageInfoBase;
        writes[i].pBufferInfo = nullptr;
        writes[i].pTexelBufferView = nullptr;

        imageInfoBase += updates[i].imageCount;
    }

    vkUpdateDescriptorSets(self->vk.device, updateCount, writes.data(), 0, nullptr);
}

static void vk_device_update_set_buffers(RDeviceObj* baseSelf, uint32_t updateCount, const RSetBufferUpdateInfo* updates)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    std::vector<VkDescriptorBufferInfo> bufferI;
    std::vector<VkWriteDescriptorSet> writes(updateCount);

    for (uint32_t i = 0; i < updateCount; i++)
    {
        const RSetBufferUpdateInfo& update = updates[i];

        for (uint32_t j = 0; j < update.bufferCount; j++)
        {
            RBufferVKObj* bufferObj = static_cast<RBufferVKObj*>(update.buffers[j].unwrap());

            bufferI.push_back({
                .buffer = bufferObj->vk.handle,
                .offset = 0,
                .range = bufferObj->info.size,
            });
        }
    }

    uint32_t bufferInfoBase = 0;

    for (uint32_t i = 0; i < updateCount; i++)
    {
        VkDescriptorType descriptorType;
        RUtil::cast_binding_type_vk(updates[i].bufferBindingType, descriptorType);

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].pNext = nullptr;
        writes[i].dstSet = static_cast<const RSetVKObj*>(updates[i].set.unwrap())->vk.handle;
        writes[i].dstBinding = updates[i].dstBinding;
        writes[i].dstArrayElement = updates[i].dstArrayIndex;
        writes[i].descriptorCount = updates[i].bufferCount;
        writes[i].descriptorType = descriptorType;
        writes[i].pImageInfo = nullptr;
        writes[i].pBufferInfo = bufferI.data() + bufferInfoBase;
        writes[i].pTexelBufferView = nullptr;

        bufferInfoBase += updates[i].bufferCount;
    }

    vkUpdateDescriptorSets(self->vk.device, updateCount, writes.data(), 0, nullptr);
}

static uint32_t vk_device_next_frame(RDeviceObj* baseSelf, RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    VulkanFrame* frame = sVulkanFrames + self->frameIndex;
    VkSemaphore imageAcquiredSemaphore = static_cast<RSemaphoreVKObj*>(frame->imageAcquired.unwrap())->vk.handle;
    VkFence frameCompleteFence = static_cast<RFenceVKObj*>(frame->frameComplete.unwrap())->vk.handle;

    {
        LD_PROFILE_SCOPE_NAME("vkWaitForFences");
        VK_CHECK(vkWaitForFences(self->vk.device, 1, &frameCompleteFence, VK_TRUE, UINT64_MAX));
    }

    VkResult acquireResult = acquire_next_image(self, imageAcquiredSemaphore);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        invalidate_swapchain(self);

        imageAcquiredSemaphore = static_cast<RSemaphoreVKObj*>(frame->imageAcquired.unwrap())->vk.handle;
        frameCompleteFence = static_cast<RFenceVKObj*>(frame->frameComplete.unwrap())->vk.handle;

        // try again with the new swapchain and synchronization primitives
        acquireResult = acquire_next_image(self, imageAcquiredSemaphore);
    }

    if (acquireResult != VK_SUCCESS)
    {
        sLog.error("vkAcquireNextImageKHR: unable to recover from VkResult {}", (int)acquireResult);
        LD_UNREACHABLE;
        return -1;
    }

    VK_CHECK(vkResetFences(self->vk.device, 1, &frameCompleteFence));

    imageAcquired = frame->imageAcquired;
    presentReady = frame->presentReady;
    frameComplete = frame->frameComplete;

    return self->vk.imageIdx;
}

static void vk_device_present_frame(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    auto* queueObj = (RQueueVKObj*)self->vk.queuePresent.unwrap();
    VulkanFrame* frame = sVulkanFrames + self->frameIndex;
    VkSemaphore presentReadySemaphore = static_cast<RSemaphoreVKObj*>(frame->presentReady.unwrap())->vk.handle;

    VkPresentInfoKHR presentI{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentReadySemaphore,
        .swapchainCount = 1,
        .pSwapchains = &self->vk.swapchain.handle,
        .pImageIndices = &self->vk.imageIdx,
    };

    VkQueue queueHandle = queueObj->vk.handle;

    // NOTE: this may or may not block, depending on the implementation and
    //       the selected swapchain present mode.
    VkResult presentResult = vkQueuePresentKHR(queueHandle, &presentI);

    // for suboptimal and out-of-date errors, we will invalidate the swapchain
    // in the following call to vk_device_next_frame().
    if (presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        return;

    if (presentResult != VK_SUCCESS)
    {
        sLog.error("unable to recover from vkQueuePresentKHR error {}", (int)presentResult);
        LD_UNREACHABLE;
    }
}

static void vk_device_get_depth_stencil_formats(RDeviceObj* baseSelf, RFormat* formats, uint32_t& count)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    count = (uint32_t)self->vk.pdevice.depthStencilFormats.size();

    if (!formats)
        return;

    for (uint32_t i = 0; i < count; i++)
        RUtil::cast_format_from_vk(self->vk.pdevice.depthStencilFormats[i], formats[i]);
}

static RSampleCountBit vk_device_get_max_sample_count(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    RSampleCountBit sampleCount;
    VkSampleCountFlagBits vkSampleCount = (VkSampleCountFlagBits)self->vk.pdevice.msaaCount;
    RUtil::cast_sample_count_from_vk(vkSampleCount, sampleCount);

    return sampleCount;
}

static RFormat vk_device_get_swapchain_color_format(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;
    RFormat format;
    RUtil::cast_format_from_vk(self->vk.swapchain.info.imageFormat, format);

    return format;
}

static RImage vk_device_get_swapchain_color_attachment(RDeviceObj* baseSelf, uint32_t imageIdx)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    return self->vk.swapchain.colorAttachments[imageIdx];
}

static uint32_t vk_device_get_swapchain_image_count(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    return (uint32_t)self->vk.swapchain.images.size();
}

static void vk_device_get_swapchain_extent(RDeviceObj* baseSelf, uint32_t* width, uint32_t* height)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    if (width)
        *width = self->vk.swapchain.width;

    if (height)
        *height = self->vk.swapchain.height;
}

static uint32_t vk_device_get_frames_in_flight_count(RDeviceObj* self)
{
    return FRAMES_IN_FLIGHT;
}

static RQueue vk_device_get_graphics_queue(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    return self->vk.queueGraphics;
}

static void vk_device_wait_idle(RDeviceObj* baseSelf)
{
    auto* self = (RDeviceVKObj*)baseSelf;

    VK_CHECK(vkDeviceWaitIdle(self->vk.device));
}

static void vk_buffer_map(RBufferObj* baseSelf)
{
    auto* self = (RBufferVKObj*)baseSelf;
    RDeviceVKObj* deviceObj = (RDeviceVKObj*)self->device.unwrap();

    VK_CHECK(vmaMapMemory(deviceObj->vk.vma, self->vk.vma, &self->hostMap));
}

static void* vk_buffer_map_read(RBufferObj* baseSelf, uint64_t offset, uint64_t size)
{
    auto* self = (RBufferVKObj*)baseSelf;
    char* src = (char*)self->hostMap + offset;

    return (void*)src;
}

static void vk_buffer_map_write(RBufferObj* baseSelf, uint64_t offset, uint64_t size, const void* data)
{
    auto* self = (RBufferVKObj*)baseSelf;
    char* dst = (char*)self->hostMap + offset;

    memcpy(dst, data, size);
}

static void vk_buffer_unmap(RBufferObj* baseSelf)
{
    auto* self = (RBufferVKObj*)baseSelf;
    RDeviceVKObj* deviceObj = (RDeviceVKObj*)self->device.unwrap();

    vmaUnmapMemory(deviceObj->vk.vma, self->vk.vma);
}

static void vk_command_list_begin(RCommandListObj* baseSelf, bool oneTimeSubmit)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkCommandBufferBeginInfo beginBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : (VkCommandBufferUsageFlags)0,
        .pInheritanceInfo = nullptr,
    };

    VK_CHECK(vkBeginCommandBuffer(self->vk.handle, &beginBI));
}

static void vk_command_list_end(RCommandListObj* baseSelf)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VK_CHECK(vkEndCommandBuffer(self->vk.handle));
}

static void vk_command_list_reset(RCommandListObj* baseSelf)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VK_CHECK(vkResetCommandBuffer(self->vk.handle, 0));
}

static void vk_command_list_cmd_begin_pass(RCommandListObj* baseSelf, const RPassBeginInfo& passBI, RFramebufferObj* baseFBObj)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkRect2D renderArea;
    renderArea.offset.x = 0;
    renderArea.offset.y = 0;
    renderArea.extent.width = passBI.width;
    renderArea.extent.height = passBI.height;

    std::vector<VkClearValue> clearValues(passBI.colorAttachmentCount);
    for (uint32_t i = 0; i < passBI.colorAttachmentCount; i++)
    {
        if (passBI.pass.colorAttachments[i].colorLoadOp == RATTACHMENT_LOAD_OP_CLEAR)
            RUtil::cast_clear_color_value_vk(passBI.clearColors[i], clearValues[i].color);
    }

    if (passBI.depthStencilAttachment && passBI.pass.depthStencilAttachment->depthLoadOp == RATTACHMENT_LOAD_OP_CLEAR)
    {
        VkClearValue clearValue;
        clearValue.depthStencil.depth = passBI.clearDepthStencil.depth;
        clearValue.depthStencil.stencil = passBI.clearDepthStencil.stencil;
        clearValues.push_back(clearValue);
    }

    RPassObj* passObj = self->deviceObj->get_or_create_pass_obj(passBI.pass);

    VkRenderPassBeginInfo vkBI{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = static_cast<RPassVKObj*>(passObj)->vk.handle,
        .framebuffer = static_cast<RFramebufferVKObj*>(baseFBObj)->vk.handle,
        .renderArea = renderArea,
        .clearValueCount = (uint32_t)clearValues.size(),
        .pClearValues = clearValues.data(),
    };

    vkCmdBeginRenderPass(self->vk.handle, &vkBI, VK_SUBPASS_CONTENTS_INLINE);

    // NOTE: By default all draw calls will apply to the full framebuffer extent
    //       unless specified otherwise, in which case the user is responsible for
    //       tracking viewport and scissor state for the remaining duration of the pass.
    VkViewport viewport = RUtil::make_viewport(renderArea.extent.width, renderArea.extent.height);
    VkRect2D scissor = RUtil::make_scissor(renderArea.extent.width, renderArea.extent.height);

    vkCmdSetViewport(self->vk.handle, 0, 1, &viewport);
    vkCmdSetScissor(self->vk.handle, 0, 1, &scissor);
}

static void vk_command_list_cmd_push_constant(RCommandListObj* baseSelf, RPipelineLayoutObj* baseLayoutObj, uint32_t offset, uint32_t size, const void* data)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    auto* layoutObj = (RPipelineLayoutVKObj*)baseLayoutObj;

    vkCmdPushConstants(self->vk.handle, layoutObj->vk.handle, VK_SHADER_STAGE_ALL, offset, size, data);
}

static void vk_command_list_cmd_bind_graphics_pipeline(RCommandListObj* baseSelf, RPipeline pipeline)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    auto* pipelineObj = (RPipelineVKObj*)pipeline.unwrap();

    LD_ASSERT(pipelineObj->vk.handles.contains(pipelineObj->vk.variantHash));
    VkPipeline vkHandle = pipelineObj->vk.handles[pipelineObj->vk.variantHash];

    vkCmdBindPipeline(self->vk.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vkHandle);

    vkCmdSetDepthTestEnable(self->vk.handle, (VkBool32)pipelineObj->variant.depthTestEnabled);
}

static void vk_command_list_cmd_bind_graphics_sets(RCommandListObj* baseSelf, RPipelineLayoutObj* baseLayoutObj, uint32_t setStart, uint32_t setCount, RSet* sets)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    auto* layoutObj = (RPipelineLayoutVKObj*)baseLayoutObj;

    std::vector<VkDescriptorSet> setHandles(setCount);
    for (uint32_t i = 0; i < setCount; i++)
    {
        auto* setObj = (RSetVKObj*)sets[i].unwrap();
        setHandles[i] = setObj->vk.handle;
    }

    vkCmdBindDescriptorSets(self->vk.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutObj->vk.handle, setStart, setCount, setHandles.data(), 0, nullptr);
}

static void vk_command_list_cmd_bind_compute_pipeline(RCommandListObj* baseSelf, RPipeline pipeline)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    auto* pipelineObj = (RPipelineVKObj*)pipeline.unwrap();

    LD_ASSERT(pipelineObj->vk.handles.contains(0));
    VkPipeline vkHandle = pipelineObj->vk.handles[0];

    vkCmdBindPipeline(self->vk.handle, VK_PIPELINE_BIND_POINT_COMPUTE, vkHandle);
}

static void vk_command_list_cmd_bind_compute_sets(RCommandListObj* baseSelf, RPipelineLayoutObj* baseLayoutObj, uint32_t setStart, uint32_t setCount, RSet* sets)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    auto* layoutObj = (RPipelineLayoutVKObj*)baseLayoutObj;

    std::vector<VkDescriptorSet> setHandles(setCount);
    for (uint32_t i = 0; i < setCount; i++)
    {
        auto* setObj = (RSetVKObj*)sets[i].unwrap();
        setHandles[i] = setObj->vk.handle;
    }

    vkCmdBindDescriptorSets(self->vk.handle, VK_PIPELINE_BIND_POINT_COMPUTE, layoutObj->vk.handle, setStart, setCount, setHandles.data(), 0, nullptr);
}

static void vk_command_list_cmd_bind_vertex_buffers(RCommandListObj* baseSelf, uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    std::vector<VkBuffer> bufferHandles(bindingCount);
    std::vector<VkDeviceSize> bufferOffsets(bindingCount);

    for (uint32_t i = 0; i < bindingCount; i++)
    {
        auto* bufferObj = (RBufferVKObj*)buffers[i].unwrap();
        bufferHandles[i] = bufferObj->vk.handle;
        bufferOffsets[i] = 0;
    }

    vkCmdBindVertexBuffers(self->vk.handle, firstBinding, bindingCount, bufferHandles.data(), bufferOffsets.data());
}

static void vk_command_list_cmd_bind_index_buffer(RCommandListObj* baseSelf, RBuffer buffer, RIndexType indexType)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    auto* bufferObj = (RBufferVKObj*)buffer.unwrap();
    VkBuffer bufferHandle = bufferObj->vk.handle;
    VkIndexType vkIndexType;

    RUtil::cast_index_type_vk(indexType, vkIndexType);

    vkCmdBindIndexBuffer(self->vk.handle, bufferHandle, 0, vkIndexType);
}

static void vk_command_list_cmd_dispatch(RCommandListObj* baseSelf, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    vkCmdDispatch(self->vk.handle, groupCountX, groupCountY, groupCountZ);
}

static void vk_command_list_cmd_set_scissor(RCommandListObj* baseSelf, const Rect& scissor)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkRect2D vkScissor = RUtil::make_scissor(scissor);
    vkCmdSetScissor(self->vk.handle, 0, 1, &vkScissor);
}

static void vk_command_list_cmd_draw(RCommandListObj* baseSelf, const RDrawInfo& drawI)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    vkCmdDraw(self->vk.handle, drawI.vertexCount, drawI.instanceCount, drawI.vertexStart, drawI.instanceStart);
}

static void vk_command_list_cmd_draw_indexed(RCommandListObj* baseSelf, const RDrawIndexedInfo& drawI)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    vkCmdDrawIndexed(self->vk.handle, drawI.indexCount, drawI.instanceCount, drawI.indexStart, drawI.vertexOffset, drawI.instanceStart);
}

static void vk_command_list_cmd_draw_indirect(RCommandListObj* baseSelf, const RDrawIndirectInfo& drawI)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    auto* bufferObj = (RBufferVKObj*)drawI.indirectBuffer.unwrap();

    vkCmdDrawIndirect(self->vk.handle, bufferObj->vk.handle, (VkDeviceSize)drawI.offset, drawI.infoCount, drawI.stride);
}

static void vk_command_list_cmd_draw_indexed_indirect(RCommandListObj* baseSelf, const RDrawIndexedIndirectInfo& drawI)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    auto* bufferObj = (RBufferVKObj*)drawI.indirectBuffer.unwrap();

    vkCmdDrawIndexedIndirect(self->vk.handle, bufferObj->vk.handle, (VkDeviceSize)drawI.offset, drawI.infoCount, drawI.stride);
}

static void vk_command_list_cmd_end_pass(RCommandListObj* baseSelf)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    vkCmdEndRenderPass(self->vk.handle);
}

static void vk_command_list_cmd_buffer_memory_barrier(RCommandListObj* baseSelf, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkPipelineStageFlags vkSrcStages;
    VkPipelineStageFlags vkDstStages;
    VkAccessFlags vkSrcAccess;
    VkAccessFlags vkDstAccess;

    RUtil::cast_pipeline_stage_flags_vk(srcStages, vkSrcStages);
    RUtil::cast_pipeline_stage_flags_vk(dstStages, vkDstStages);
    RUtil::cast_access_flags_vk(barrier.srcAccess, vkSrcAccess);
    RUtil::cast_access_flags_vk(barrier.dstAccess, vkDstAccess);

    VkBufferMemoryBarrier vkBarrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = vkSrcAccess,
        .dstAccessMask = vkDstAccess,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = static_cast<const RBufferVKObj*>(barrier.buffer.unwrap())->vk.handle,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    vkCmdPipelineBarrier(self->vk.handle, vkSrcStages, vkDstStages, 0, 0, nullptr, 1, &vkBarrier, 0, nullptr);
}

static void vk_command_list_cmd_image_memory_barrier(RCommandListObj* baseSelf, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkPipelineStageFlags vkSrcStages;
    VkPipelineStageFlags vkDstStages;
    VkImageLayout vkOldLayout;
    VkImageLayout vkNewLayout;
    VkAccessFlags vkSrcAccess;
    VkAccessFlags vkDstAccess;
    VkImageAspectFlags vkAspect;

    RUtil::cast_pipeline_stage_flags_vk(srcStages, vkSrcStages);
    RUtil::cast_pipeline_stage_flags_vk(dstStages, vkDstStages);
    RUtil::cast_image_layout_vk(barrier.oldLayout, vkOldLayout);
    RUtil::cast_image_layout_vk(barrier.newLayout, vkNewLayout);
    RUtil::cast_access_flags_vk(barrier.srcAccess, vkSrcAccess);
    RUtil::cast_access_flags_vk(barrier.dstAccess, vkDstAccess);
    RUtil::cast_format_image_aspect_vk(barrier.image.format(), vkAspect);

    VkImageSubresourceRange range{
        .aspectMask = vkAspect,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    VkImageMemoryBarrier vkBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = vkSrcAccess,
        .dstAccessMask = vkDstAccess,
        .oldLayout = vkOldLayout,
        .newLayout = vkNewLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = static_cast<const RImageVKObj*>(barrier.image.unwrap())->vk.handle,
        .subresourceRange = range,
    };

    vkCmdPipelineBarrier(self->vk.handle, vkSrcStages, vkDstStages, 0, 0, nullptr, 0, nullptr, 1, &vkBarrier);
}

static void vk_command_list_cmd_copy_buffer(RCommandListObj* baseSelf, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkBuffer srcBufferHandle = static_cast<RBufferVKObj*>(srcBuffer.unwrap())->vk.handle;
    VkBuffer dstBufferHandle = static_cast<RBufferVKObj*>(dstBuffer.unwrap())->vk.handle;

    std::vector<VkBufferCopy> copies(regionCount);
    for (uint32_t i = 0; i < regionCount; i++)
    {
        copies[i].srcOffset = regions[i].srcOffset;
        copies[i].dstOffset = regions[i].dstOffset;
        copies[i].size = regions[i].size;
    }

    vkCmdCopyBuffer(self->vk.handle, srcBufferHandle, dstBufferHandle, regionCount, copies.data());
}

static void vk_command_list_cmd_copy_buffer_to_image(RCommandListObj* baseSelf, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkBuffer srcBufferHandle = static_cast<RBufferVKObj*>(srcBuffer.unwrap())->vk.handle;
    VkImage dstImageHandle = static_cast<RImageVKObj*>(dstImage.unwrap())->vk.handle;
    VkImageLayout vkLayout;
    VkImageAspectFlags vkAspects;

    RUtil::cast_image_layout_vk(dstImageLayout, vkLayout);
    RUtil::cast_format_image_aspect_vk(dstImage.format(), vkAspects);

    std::vector<VkBufferImageCopy> copies(regionCount);
    for (uint32_t i = 0; i < regionCount; i++)
    {
        copies[i].bufferOffset = regions[i].bufferOffset;
        copies[i].bufferRowLength = 0;
        copies[i].bufferImageHeight = 0;
        copies[i].imageExtent.width = regions[i].imageWidth;
        copies[i].imageExtent.height = regions[i].imageHeight;
        copies[i].imageExtent.depth = regions[i].imageDepth;
        copies[i].imageSubresource.aspectMask = vkAspects;
        copies[i].imageSubresource.baseArrayLayer = 0;
        copies[i].imageSubresource.layerCount = regions[i].imageLayers;
        copies[i].imageSubresource.mipLevel = 0;
    }

    vkCmdCopyBufferToImage(self->vk.handle, srcBufferHandle, dstImageHandle, vkLayout, regionCount, copies.data());
}

static void vk_command_list_cmd_copy_image_to_buffer(RCommandListObj* baseSelf, RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions)
{
    auto* self = (RCommandListVKObj*)baseSelf;

    VkBuffer dstBufferHandle = static_cast<RBufferVKObj*>(dstBuffer.unwrap())->vk.handle;
    VkImage srcImageHandle = static_cast<RImageVKObj*>(srcImage.unwrap())->vk.handle;
    VkImageLayout vkLayout;
    VkImageAspectFlags vkAspects;

    RUtil::cast_image_layout_vk(srcImageLayout, vkLayout);
    RUtil::cast_format_image_aspect_vk(srcImage.format(), vkAspects);

    std::vector<VkBufferImageCopy> copies(regionCount);
    for (uint32_t i = 0; i < regionCount; i++)
    {
        copies[i].bufferOffset = regions[i].bufferOffset;
        copies[i].bufferRowLength = 0;
        copies[i].bufferImageHeight = 0;
        copies[i].imageExtent.width = regions[i].imageWidth;
        copies[i].imageExtent.height = regions[i].imageHeight;
        copies[i].imageExtent.depth = regions[i].imageDepth;
        copies[i].imageSubresource.aspectMask = vkAspects;
        copies[i].imageSubresource.baseArrayLayer = 0;
        copies[i].imageSubresource.layerCount = regions[i].imageLayers;
        copies[i].imageSubresource.mipLevel = 0;
    }

    vkCmdCopyImageToBuffer(self->vk.handle, srcImageHandle, vkLayout, dstBufferHandle, regionCount, copies.data());
}

static void vk_command_list_cmd_blit_image(RCommandListObj* baseSelf, RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter)
{
    auto* self = (RCommandListVKObj*)baseSelf;
    RImageVKObj* srcImageObj = (RImageVKObj*)srcImage.unwrap();
    RImageVKObj* dstImageObj = (RImageVKObj*)dstImage.unwrap();

    VkImageLayout srcLayout;
    VkImageLayout dstLayout;
    VkImageAspectFlags srcAspect;
    VkImageAspectFlags dstAspect;
    VkFilter vkFilter;
    RUtil::cast_image_layout_vk(srcImageLayout, srcLayout);
    RUtil::cast_image_layout_vk(dstImageLayout, dstLayout);
    RUtil::cast_format_image_aspect_vk(srcImage.format(), srcAspect);
    RUtil::cast_format_image_aspect_vk(srcImage.format(), dstAspect);
    RUtil::cast_filter_vk(filter, vkFilter);

    std::vector<VkImageBlit> blits(regionCount);
    for (uint32_t i = 0; i < regionCount; i++)
    {
        blits[i].srcOffsets[0].x = regions[i].srcMinOffset.x;
        blits[i].srcOffsets[0].y = regions[i].srcMinOffset.y;
        blits[i].srcOffsets[0].z = regions[i].srcMinOffset.z;
        blits[i].srcOffsets[1].x = regions[i].srcMaxOffset.x;
        blits[i].srcOffsets[1].y = regions[i].srcMaxOffset.y;
        blits[i].srcOffsets[1].z = regions[i].srcMaxOffset.z;
        blits[i].dstOffsets[0].x = regions[i].dstMinOffset.x;
        blits[i].dstOffsets[0].y = regions[i].dstMinOffset.y;
        blits[i].dstOffsets[0].z = regions[i].dstMinOffset.z;
        blits[i].dstOffsets[1].x = regions[i].dstMaxOffset.x;
        blits[i].dstOffsets[1].y = regions[i].dstMaxOffset.y;
        blits[i].dstOffsets[1].z = regions[i].dstMaxOffset.z;
        blits[i].srcSubresource.aspectMask = srcAspect;
        blits[i].srcSubresource.mipLevel = 0;
        blits[i].srcSubresource.baseArrayLayer = 0;
        blits[i].srcSubresource.layerCount = 1;
        blits[i].dstSubresource.aspectMask = dstAspect;
        blits[i].dstSubresource.mipLevel = 0;
        blits[i].dstSubresource.baseArrayLayer = 0;
        blits[i].dstSubresource.layerCount = 1;
    }

    vkCmdBlitImage(self->vk.handle, srcImageObj->vk.handle, srcLayout, dstImageObj->vk.handle, dstLayout, regionCount, blits.data(), vkFilter);
}

static void vk_pipeline_create_variant(RPipelineObj* baseSelf)
{
    auto* self = (RPipelineVKObj*)baseSelf;

    // the same RPipeline handle can refer to Vulkan pipelines that vary in:
    // - render passes
    // - per-attachment color write masks
    std::size_t variantHash = self->variant.passObj->hash;

    for (RColorComponentFlags& writeMasks : self->variant.colorWriteMasks)
        hash_combine(variantHash, (uint32_t)writeMasks);

    self->vk.variantHash = (uint32_t)variantHash;

    if (self->vk.handles.contains((uint32_t)variantHash))
        return;

    for (size_t i = 0; i < self->variant.colorWriteMasks.size(); i++)
        RUtil::cast_color_components_vk(self->variant.colorWriteMasks[i], self->vk.blendStates[i].colorWriteMask);

    VkSampleCountFlagBits rasterizationSamples;
    RUtil::cast_sample_count_vk(self->variant.passObj->samples, rasterizationSamples);
    VkPipelineMultisampleStateCreateInfo multisampleSCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = rasterizationSamples,
        .sampleShadingEnable = VK_FALSE,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    std::array<VkDynamicState, 3> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
    };

    VkPipelineDynamicStateCreateInfo dynamicSCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (uint32_t)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (uint32_t)self->vk.shaderStageCI.size(),
        .pStages = self->vk.shaderStageCI.data(),
        .pVertexInputState = &self->vk.vertexInputSCI,
        .pInputAssemblyState = &self->vk.inputAsmSCI,
        .pTessellationState = &self->vk.tessellationSCI,
        .pViewportState = &self->vk.viewportSCI,
        .pRasterizationState = &self->vk.rasterizationSCI,
        .pMultisampleState = &multisampleSCI,
        .pDepthStencilState = &self->vk.depthStencilSCI,
        .pColorBlendState = &self->vk.colorBlendSCI,
        .pDynamicState = &dynamicSCI,
        .layout = static_cast<RPipelineLayoutVKObj*>(self->layoutObj)->vk.handle,
        .renderPass = static_cast<RPassVKObj*>(self->variant.passObj)->vk.handle,
    };

    VkPipeline vkHandle;
    VkDevice vkDeviceHandle = static_cast<RDeviceVKObj*>(self->deviceObj)->vk.device;

    VK_CHECK(vkCreateGraphicsPipelines(vkDeviceHandle, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &vkHandle));

    self->vk.variantHash = variantHash;
    self->vk.handles[variantHash] = vkHandle;
}

static RCommandList vk_command_pool_allocate(RCommandPoolObj* baseSelf, RCommandListObj* baseListObj)
{
    auto* self = (RCommandPoolVKObj*)baseSelf;
    auto* listObj = (RCommandListVKObj*)baseListObj;

    VkCommandBufferAllocateInfo bufferAI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = self->vk.handle,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    listObj->api = &sRCommandListVKAPI;
    listObj->vk.device = self->vk.device;

    VK_CHECK(vkAllocateCommandBuffers(self->vk.device, &bufferAI, &listObj->vk.handle));

    return {listObj};
}

static void vk_command_pool_reset(RCommandPoolObj* baseSelf)
{
    auto* self = (RCommandPoolVKObj*)baseSelf;

    VK_CHECK(vkResetCommandPool(self->vk.device, self->vk.handle, 0));
}

static RSet vk_set_pool_allocate(RSetPoolObj* baseSelf, RSetObj* baseSetObj)
{
    auto* self = (RSetPoolVKObj*)baseSelf;
    auto* setObj = (RSetVKObj*)baseSetObj;
    auto* setLayoutObj = (RSetLayoutVKObj*)self->layoutObj;

    VkDescriptorSetAllocateInfo setAI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = self->vk.handle,
        .descriptorSetCount = 1,
        .pSetLayouts = &setLayoutObj->vk.handle,
    };

    VK_CHECK(vkAllocateDescriptorSets(self->vk.device, &setAI, &setObj->vk.handle));

    return {setObj};
}

static void vk_set_pool_reset(RSetPoolObj* baseSelf)
{
    auto* self = (RSetPoolVKObj*)baseSelf;

    VK_CHECK(vkResetDescriptorPool(self->vk.device, self->vk.handle, 0));
}

static void vk_queue_wait_idle(RQueueObj* baseSelf)
{
    auto* self = (RQueueVKObj*)baseSelf;

    VK_CHECK(vkQueueWaitIdle(self->vk.handle));
}

static void vk_queue_submit(RQueueObj* baseSelf, const RSubmitInfo& submitI, RFence fence)
{
    auto* self = (RQueueVKObj*)baseSelf;
    VkFence fenceHandle = fence ? static_cast<RFenceVKObj*>(fence.unwrap())->vk.handle : VK_NULL_HANDLE;

    std::vector<VkSemaphore> semaphoreHandles(submitI.waitCount + submitI.signalCount);

    uint32_t i;

    for (i = 0; i < submitI.waitCount; i++)
        semaphoreHandles[i] = static_cast<RSemaphoreVKObj*>(submitI.waits[i].unwrap())->vk.handle;

    for (i = 0; i < submitI.signalCount; i++)
        semaphoreHandles[submitI.waitCount + i] = static_cast<RSemaphoreVKObj*>(submitI.signals[i].unwrap())->vk.handle;

    std::vector<VkCommandBuffer> commandHandles(submitI.listCount);

    for (i = 0; i < submitI.listCount; i++)
        commandHandles[i] = static_cast<RCommandListVKObj*>(submitI.lists[i].unwrap())->vk.handle;

    std::vector<VkPipelineStageFlags> waitStages(submitI.waitCount);
    for (i = 0; i < submitI.waitCount; i++)
        RUtil::cast_pipeline_stage_flags_vk(submitI.waitStages[i], waitStages[i]);

    VkSubmitInfo submit{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = submitI.waitCount,
        .pWaitSemaphores = semaphoreHandles.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = (uint32_t)commandHandles.size(),
        .pCommandBuffers = commandHandles.data(),
        .signalSemaphoreCount = submitI.signalCount,
        .pSignalSemaphores = semaphoreHandles.data() + submitI.waitCount,
    };

    VK_CHECK(vkQueueSubmit(self->vk.handle, 1, &submit, fenceHandle));
}

static VkResult acquire_next_image(RDeviceVKObj* obj, VkSemaphore imageAcquiredSemaphore)
{
    LD_PROFILE_SCOPE_NAME("vkAcquireNextImageKHR");

    return vkAcquireNextImageKHR(
        obj->vk.device,
        obj->vk.swapchain.handle,
        UINT64_MAX,
        imageAcquiredSemaphore,
        VK_NULL_HANDLE,
        &obj->vk.imageIdx);
}

static void choose_physical_device(RDeviceVKObj* obj)
{
    std::vector<VkPhysicalDevice> handles;

    uint32_t handleCount;
    VK_CHECK(vkEnumeratePhysicalDevices(obj->vk.instance, &handleCount, nullptr));
    handles.resize(handleCount);
    VK_CHECK(vkEnumeratePhysicalDevices(obj->vk.instance, &handleCount, handles.data()));

    PhysicalDevice& pdevice = obj->vk.pdevice;

    for (VkPhysicalDevice& handle : handles)
    {
        bool chosen = true; // TODO:

        obj->vk.pdevice.handle = handle;

        vkGetPhysicalDeviceProperties(pdevice.handle, &pdevice.deviceProps);
        sLog.info("VkPhysicalDevice: {}", pdevice.deviceProps.deviceName);

        const VkPhysicalDeviceLimits& limits = pdevice.deviceProps.limits;

        VkSampleCountFlags count = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
        pdevice.msaaCount = VK_SAMPLE_COUNT_1_BIT;
        if (count & VK_SAMPLE_COUNT_64_BIT)
            pdevice.msaaCount = VK_SAMPLE_COUNT_64_BIT;
        else if (count & VK_SAMPLE_COUNT_32_BIT)
            pdevice.msaaCount = VK_SAMPLE_COUNT_32_BIT;
        else if (count & VK_SAMPLE_COUNT_16_BIT)
            pdevice.msaaCount = VK_SAMPLE_COUNT_16_BIT;
        else if (count & VK_SAMPLE_COUNT_8_BIT)
            pdevice.msaaCount = VK_SAMPLE_COUNT_8_BIT;
        else if (count & VK_SAMPLE_COUNT_4_BIT)
            pdevice.msaaCount = VK_SAMPLE_COUNT_4_BIT;
        else if (count & VK_SAMPLE_COUNT_2_BIT)
            pdevice.msaaCount = VK_SAMPLE_COUNT_2_BIT;

        vkGetPhysicalDeviceFeatures(pdevice.handle, &pdevice.deviceFeatures);

        // queue families on this physical device
        uint32_t familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &familyCount, nullptr);
        pdevice.familyProps.resize(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &familyCount, pdevice.familyProps.data());

        if (obj->vk.surface != VK_NULL_HANDLE)
        {
            VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle, obj->vk.surface, &pdevice.surfaceCaps));

            // available surface formats on this physical device
            uint32_t formatCount;
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(handle, obj->vk.surface, &formatCount, nullptr));
            pdevice.surfaceFormats.resize(formatCount);
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(handle, obj->vk.surface, &formatCount, pdevice.surfaceFormats.data()));
        }

        // available depth stencil formats on this physical device
        VkFormatProperties formatProps;
        std::array<VkFormat, 2> depthStencilCandidates = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        };
        for (VkFormat candidate : depthStencilCandidates)
        {
            vkGetPhysicalDeviceFormatProperties(handle, candidate, &formatProps);
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                pdevice.depthStencilFormats.push_back(candidate);
        }

        // present modes on this physical device
        if (obj->vk.surface != VK_NULL_HANDLE)
        {
            uint32_t modeCount;
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(handle, obj->vk.surface, &modeCount, NULL));
            pdevice.presentModes.resize(modeCount);
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(handle, obj->vk.surface, &modeCount, pdevice.presentModes.data()));
        }

        if (chosen)
            break;
    }
}

static void configure_swapchain(RDeviceVKObj* obj, SwapchainInfo* swapchainI)
{
    PhysicalDevice& pdevice = obj->vk.pdevice;

    // configure color format
    LD_ASSERT(!pdevice.surfaceFormats.empty());
    swapchainI->imageFormat = pdevice.surfaceFormats[0].format;
    swapchainI->imageColorSpace = pdevice.surfaceFormats[0].colorSpace;

    for (VkSurfaceFormatKHR& surfaceFmt : pdevice.surfaceFormats)
    {
        if (surfaceFmt.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchainI->imageFormat = surfaceFmt.format;
            swapchainI->imageColorSpace = surfaceFmt.colorSpace;
            break;
        }
    }

    // configure present mode
    swapchainI->presentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed support; vsynced

    for (VkPresentModeKHR& mode : pdevice.presentModes)
    {
        if (swapchainI->vsyncHint && mode == VK_PRESENT_MODE_MAILBOX_KHR) // preferred vsync mode
        {
            swapchainI->presentMode = mode;
            break;
        }

        if (!swapchainI->vsyncHint && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) // preferred non-vsync mode
        {
            swapchainI->presentMode = mode;
            break;
        }
    }
}

static void create_swapchain(RDeviceVKObj* obj, const SwapchainInfo& swapchainI)
{
    LD_PROFILE_SCOPE;

    PhysicalDevice& pdevice = obj->vk.pdevice;
    Swapchain& swp = obj->vk.swapchain;

    swp.info = swapchainI;

    constexpr uint32_t swapchainImageHint = 3;

    uint32_t surfaceMinImageCount = pdevice.surfaceCaps.minImageCount;
    uint32_t surfaceMaxImageCount = pdevice.surfaceCaps.maxImageCount; // may be zero if there is no upper limit

    // NOTE: we require a minimum of surfaceMinImageCount + 1 to prevent driver code from blocking.
    //       i.e. if there are 3 swapchain images we can acquire 2 images without blocking.
    uint32_t minImageCount = std::max<uint32_t>(surfaceMinImageCount + 1, swapchainImageHint);
    if (surfaceMaxImageCount > 0 && minImageCount > surfaceMaxImageCount)
        minImageCount = surfaceMaxImageCount; // clamp to upper limit

    VkExtent2D imageExtent = pdevice.surfaceCaps.currentExtent;
    if (imageExtent.width == UINT32_MAX || imageExtent.height == UINT32_MAX)
    {
        // if driver hasn't updated current surface extent, grab extent from glfw.
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(obj->glfw, &fbWidth, &fbHeight);

        imageExtent.width = (uint32_t)fbWidth;
        imageExtent.height = (uint32_t)fbHeight;
    }

    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = obj->vk.surface,
        .minImageCount = minImageCount,
        .imageFormat = swapchainI.imageFormat,
        .imageColorSpace = swapchainI.imageColorSpace,
        .imageExtent = imageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // TODO: transfer dst not guaranteed
        .preTransform = pdevice.surfaceCaps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = swapchainI.presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    std::array<uint32_t, 2> familyIndices{obj->vk.familyIdxGraphics, obj->vk.familyIdxPresent};

    if (obj->vk.familyIdxGraphics == obj->vk.familyIdxPresent)
    {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.queueFamilyIndexCount = 0;
    }
    else
    {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCI.queueFamilyIndexCount = (uint32_t)familyIndices.size();
        swapchainCI.pQueueFamilyIndices = familyIndices.data();
    }

    VK_CHECK(vkCreateSwapchainKHR(obj->vk.device, &swapchainCI, nullptr, &swp.handle));

    uint32_t imageCount;
    VK_CHECK(vkGetSwapchainImagesKHR(obj->vk.device, swp.handle, &imageCount, nullptr));
    swp.images.resize(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(obj->vk.device, swp.handle, &imageCount, swp.images.data()));

    VkExtent2D swpExtent = swapchainCI.imageExtent;

    // create RImage color attachments that can be used to create a swapchain framebuffer
    swp.colorAttachments.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
        swp.colorAttachments[i] = create_swapchain_color_attachment(obj, swp.images[i], swp.info.imageFormat, swpExtent.width, swpExtent.height);

    swp.width = swpExtent.width;
    swp.height = swpExtent.height;

    std::string presentMode;
    RUtil::print_vk_present_mode(swp.info.presentMode, presentMode);

    sLog.info("Vulkan swapchain {}x{} with {} images (hint {}, min {}, max {}) {}",
              (int)swp.width,
              (int)swp.height,
              (int)swp.images.size(),
              (int)swapchainImageHint,
              (int)surfaceMinImageCount,
              (int)surfaceMaxImageCount,
              presentMode);
}

static RImage create_swapchain_color_attachment(RDeviceVKObj* deviceObj, VkImage image, VkFormat colorFormat, uint32_t width, uint32_t height)
{
    auto* obj = (RImageVKObj*)heap_new<RImageVKObj>(MEMORY_USAGE_RENDER);
    obj->rid = RObjectID::get();
    obj->vk.handle = image;
    obj->vk.vma = nullptr; // unrelated to VMA

    RFormat format;
    RUtil::cast_format_from_vk(colorFormat, format);
    obj->info.format = format;
    obj->info.width = width;
    obj->info.height = height;
    obj->info.depth = 1;
    obj->info.type = RIMAGE_TYPE_2D;
    obj->info.usage = RIMAGE_USAGE_COLOR_ATTACHMENT_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT; // TODO: transfer dst not guaranteed

    VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    VkImageViewCreateInfo viewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = colorFormat,
        .subresourceRange = range,
    };

    VK_CHECK(vkCreateImageView(deviceObj->vk.device, &viewCI, nullptr, &obj->vk.viewHandle));

    return {obj};
}

static void destroy_swapchain(RDeviceVKObj* obj)
{
    for (RImage attachment : obj->vk.swapchain.colorAttachments)
        destroy_swapchain_color_attachment(obj, attachment);

    vkDestroySwapchainKHR(obj->vk.device, obj->vk.swapchain.handle, nullptr);
}

static void destroy_swapchain_color_attachment(RDeviceVKObj* deviceObj, RImage attachment)
{
    auto* obj = (RImageVKObj*)attachment.unwrap();

    vkDestroyImageView(deviceObj->vk.device, obj->vk.viewHandle, nullptr);

    heap_delete<RImageVKObj>(obj);
}

static void invalidate_swapchain(RDeviceVKObj* obj)
{
    // wait until all frames in flight complete.
    vkDeviceWaitIdle(obj->vk.device);

    // invalidate swapchain
    SwapchainInfo swapchainI = obj->vk.swapchain.info;
    PhysicalDevice& pdevice = obj->vk.pdevice;
    size_t oldImageCount = obj->vk.swapchain.colorAttachments.size();

    destroy_swapchain(obj);

    // update surface capabilities, we should create a new swapchain using the latest
    // VkSurfaceCapabilitiesKHR::currentExtent as swapchain image extent
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice.handle, obj->vk.surface, &pdevice.surfaceCaps));

    create_swapchain(obj, swapchainI);

    size_t newImageCount = obj->vk.swapchain.colorAttachments.size();

    if (newImageCount != oldImageCount)
    {
        sLog.warn("invalidated swapchain but image count changes from {} to {}", oldImageCount, newImageCount);
        LD_UNREACHABLE;
        return;
    }

    // invalidate frames in flight synchronization
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        VulkanFrame* frame = sVulkanFrames + i;

        vk_device_destroy_semaphore(obj, frame->presentReady);
        vk_device_destroy_semaphore(obj, frame->imageAcquired);
        vk_device_destroy_fence(obj, frame->frameComplete);
        frame->presentReady = vk_device_create_semaphore(obj, &frame->presentReadyObj);
        frame->imageAcquired = vk_device_create_semaphore(obj, &frame->imageAcquiredObj);
        frame->frameComplete = vk_device_create_fence(obj, true, &frame->frameCompleteObj);
        frame->presentReadyObj.rid = RObjectID::get();
        frame->imageAcquiredObj.rid = RObjectID::get();
        frame->frameCompleteObj.rid = RObjectID::get();
    }
}

static void create_vma_allocator(RDeviceVKObj* obj)
{
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCI{
        .physicalDevice = obj->vk.pdevice.handle,
        .device = obj->vk.device,
        .pVulkanFunctions = &vulkanFunctions,
        .instance = obj->vk.instance,
        .vulkanApiVersion = API_VERSION,
    };

    VK_CHECK(vmaCreateAllocator(&allocatorCI, &obj->vk.vma));
}

static void destroy_vma_allocator(RDeviceVKObj* obj)
{
    vmaDestroyAllocator(obj->vk.vma);
}

static RQueue create_queue(uint32_t queueFamilyIdx, VkQueue handle)
{
    auto* obj = heap_new<RQueueVKObj>(MEMORY_USAGE_RENDER);
    obj->vk.familyIdx = queueFamilyIdx;
    obj->vk.handle = handle;

    return {obj};
}

static void destroy_queue(RQueue queue)
{
    auto* obj = (RQueueVKObj*)queue.unwrap();

    heap_delete<RQueueVKObj>(obj);
}

} // namespace LD
