#include "RBackendObj.h"
#include "RShaderCompiler.h"
#include "RUtilInternal.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Memory.h>
#include <array>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // hide from user
#define VMA_VULKAN_VERSION 1003000
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h> // hide from user

#define VK_CHECK(CALL)                                             \
    do                                                             \
    {                                                              \
        VkResult result_ = CALL;                                   \
        if (result_ != VK_SUCCESS)                                 \
        {                                                          \
            printf("VK_CHECK failed with VkResult %d\n", result_); \
        }                                                          \
    } while (0)

// clang-format off
#define APPLICATION_NAME      "LudensVulkanBackend"
#define APPLICATION_VERSION   VK_MAKE_API_VERSION(0, 0, 0, 0)
#define API_VERSION           VK_API_VERSION_1_3
// clang-format on

#define FRAMES_IN_FLIGHT 2

namespace LD {

// @brief Vulkan frame boundary
struct VulkanFrame
{
    RFence frameComplete;
    RSemaphore imageAcquired;
    RSemaphore presentReady;
    RFenceObj frameCompleteObj;
    RSemaphoreObj imageAcquiredObj;
    RSemaphoreObj presentReadyObj;
} sVulkanFrames[FRAMES_IN_FLIGHT];

static RSemaphore vk_device_create_semaphore(RDeviceObj* self, RSemaphoreObj* obj);
static void vk_device_destroy_semaphore(RDeviceObj* self, RSemaphore semaphore);
static RFence vk_device_create_fence(RDeviceObj* self, bool createSignaled, RFenceObj* obj);
static void vk_device_destroy_fence(RDeviceObj* self, RFence fence);
static RBuffer vk_device_create_buffer(RDeviceObj* self, const RBufferInfo& bufferI, RBufferObj* obj);
static void vk_device_destroy_buffer(RDeviceObj* self, RBuffer buffer);
static RImage vk_device_create_image(RDeviceObj* self, const RImageInfo& imageI, RImageObj* obj);
static void vk_device_destroy_image(RDeviceObj* self, RImage image);
static void vk_device_create_pass(RDeviceObj* self, const RPassInfo& passI, RPassObj* obj);
static void vk_device_destroy_pass(RDeviceObj* self, RPassObj* obj);
static void vk_device_create_framebuffer(RDeviceObj* self, const RFramebufferInfo& fbI, RFramebufferObj* obj);
static void vk_device_destroy_framebuffer(RDeviceObj* self, RFramebufferObj* obj);
static RCommandPool vk_device_create_command_pool(RDeviceObj* self, const RCommandPoolInfo& poolI, RCommandPoolObj* obj);
static void vk_device_destroy_command_pool(RDeviceObj* self, RCommandPool pool);
static RShader vk_device_create_shader(RDeviceObj* self, const RShaderInfo& shaderI, RShaderObj* obj);
static void vk_device_destroy_shader(RDeviceObj* self, RShader shader);
static RSetPool vk_device_create_set_pool(RDeviceObj* self, const RSetPoolInfo& poolI, RSetPoolObj* obj);
static void vk_device_destroy_set_pool(RDeviceObj* self, RSetPool pool);
static void vk_device_create_set_layout(RDeviceObj* self, const RSetLayoutInfo& layoutI, RSetLayoutObj* obj);
static void vk_device_destroy_set_layout(RDeviceObj* self, RSetLayoutObj* layoutObj);
static void vk_device_create_pipeline_layout(RDeviceObj* self, const RPipelineLayoutInfo& layoutI, RPipelineLayoutObj* obj);
static void vk_device_destroy_pipeline_layout(RDeviceObj* self, RPipelineLayoutObj* layoutObj);
static RPipeline vk_device_create_pipeline(RDeviceObj* self, const RPipelineInfo& pipelineI, RPipelineObj* obj);
static RPipeline vk_device_create_compute_pipeline(RDeviceObj* self, const RComputePipelineInfo& pipelineI, RPipelineObj* pipelineObj);
static void vk_device_destroy_pipeline(RDeviceObj* self, RPipeline pipeline);
static void vk_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI);
static void vk_device_update_set_images(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates);
static void vk_device_update_set_buffers(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates);
static uint32_t vk_device_next_frame(RDeviceObj* self, RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete);
static void vk_device_present_frame(RDeviceObj* self);
static void vk_device_get_depth_stencil_formats(RDeviceObj* self, RFormat* formats, uint32_t& count);
static RSampleCountBit vk_device_get_max_sample_count(RDeviceObj* self);
static RFormat vk_device_get_swapchain_color_format(RDeviceObj* self);
static RImage vk_device_get_swapchain_color_attachment(RDeviceObj* self, uint32_t imageIdx);
static uint32_t vk_device_get_swapchain_image_count(RDeviceObj* self);
static uint32_t vk_device_get_frames_in_flight_count(RDeviceObj* self);
static RQueue vk_device_get_graphics_queue(RDeviceObj* self);
static void vk_device_wait_idle(RDeviceObj* self);

static void vk_buffer_map(RBufferObj* self);
static void vk_buffer_map_write(RBufferObj* self, uint64_t offset, uint64_t size, const void* data);
static void vk_buffer_unmap(RBufferObj* self);

static void vk_command_list_free(RCommandListObj* self);
static void vk_command_list_begin(RCommandListObj* self, bool oneTimeSubmit);
static void vk_command_list_end(RCommandListObj* self);
static void vk_command_list_cmd_begin_pass(RCommandListObj* self, const RPassBeginInfo& passBI);
static void vk_command_list_cmd_bind_graphics_pipeline(RCommandListObj* self, RPipeline pipeline);
static void vk_command_list_cmd_bind_graphics_sets(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t setStart, uint32_t setCount, RSet* sets);
static void vk_command_list_cmd_bind_compute_pipeline(RCommandListObj* self, RPipeline pipeline);
static void vk_command_list_cmd_bind_compute_sets(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t setStart, uint32_t setCount, RSet* sets);
static void vk_command_list_cmd_bind_vertex_buffers(RCommandListObj* self, uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);
static void vk_command_list_cmd_bind_index_buffer(RCommandListObj* self, RBuffer buffer, RIndexType indexType);
static void vk_command_list_cmd_dispatch(RCommandListObj* self, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
static void vk_command_list_cmd_draw(RCommandListObj* self, const RDrawInfo& drawI);
static void vk_command_list_cmd_draw_indexed(RCommandListObj* self, const RDrawIndexedInfo& drawI);
static void vk_command_list_cmd_end_pass(RCommandListObj* self);
static void vk_command_list_cmd_buffer_memory_barrier(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier);
static void vk_command_list_cmd_image_memory_barrier(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);
static void vk_command_list_cmd_copy_buffer(RCommandListObj* self, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);
static void vk_command_list_cmd_copy_buffer_to_image(RCommandListObj* self, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);
static void vk_command_list_cmd_copy_image_to_buffer(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions);
static void vk_command_list_cmd_blit_image(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter);

static RCommandList vk_command_pool_allocate(RCommandPoolObj* self, RCommandListObj* listObj);
static void vk_command_pool_reset(RCommandPoolObj* self);

static RSet vk_set_pool_allocate(RSetPoolObj* self, RSetObj* setObj);
static void vk_set_pool_reset(RSetPoolObj* self);

static void vk_queue_wait_idle(RQueueObj* self);
static void vk_queue_submit(RQueueObj* self, const RSubmitInfo& submitI, RFence fence);

static void choose_physical_device(RDeviceObj* obj);
static void configure_swapchain(RDeviceObj* obj, SwapchainInfo* swapchainI);
static void create_swapchain(RDeviceObj* obj, const SwapchainInfo& swapchainI);
static RImage create_swapchain_color_attachment(RDeviceObj* deviceObj, VkImage image, VkFormat colorFormat, uint32_t width, uint32_t height);
static void destroy_swapchain(RDeviceObj* obj);
static void destroy_swapchain_color_attachment(RDeviceObj* deviceObj, RImage attachment);
static void create_vma_allocator(RDeviceObj* obj);
static void destroy_vma_allocator(RDeviceObj* obj);
static RQueue create_queue(uint32_t queueFamilyIdx, VkQueue handle);
static void destroy_queue(RQueue queue);

void vk_create_device(RDeviceObj* self, const RDeviceInfo& deviceI)
{
    self->backend = RDEVICE_BACKEND_VULKAN;
    self->vk.frameIdx = 0;

    self->init_vk_api();

    new (&self->vk.pdevice) PhysicalDevice();
    new (&self->vk.swapchain) Swapchain();

    // NOTE: make sure glfwInit() is called before this
    LD_ASSERT(glfwVulkanSupported() == GLFW_TRUE);

    std::set<std::string> desiredInstanceExtSet;

    // already contains VK_KHR_surface
    uint32_t glfwExtCount;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    for (uint32_t i = 0; i < glfwExtCount; i++)
        desiredInstanceExtSet.insert(glfwExts[i]);

    // SPACE: insert any other user-requested extensions into set

    std::vector<const char*> desiredInstanceExts;
    for (const std::string& desiredExt : desiredInstanceExtSet)
        desiredInstanceExts.push_back(desiredExt.c_str());

    std::vector<const char*> desiredLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

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
        .enabledLayerCount = (uint32_t)desiredLayers.size(),
#else
        .enabledLayerCount = 0,
#endif
        .ppEnabledLayerNames = desiredLayers.data(),
        .enabledExtensionCount = (uint32_t)desiredInstanceExts.size(),
        .ppEnabledExtensionNames = desiredInstanceExts.data(),
    };

    VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &self->vk.instance));

    // delegate surface creation to GLFW
    VK_CHECK(glfwCreateWindowSurface(self->vk.instance, deviceI.window, nullptr, &self->vk.surface));

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

        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(pdevice.handle, idx, self->vk.surface, &supported);
        if (familyIdxPresent == familyCount && supported)
            familyIdxPresent = idx;
    }

    LD_ASSERT(familyIdxGraphics != familyCount && "graphics queue family not found");
    LD_ASSERT(familyIdxTransfer != familyCount && "transfer queue family not found");
    LD_ASSERT(familyIdxCompute != familyCount && "compute queue family not found");
    LD_ASSERT(familyIdxPresent != familyCount && "present queue family not found");

    std::string queueFlags;
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxGraphics].queueFlags, queueFlags);
    printf("Vulkan graphics queue family index %d: (%s)\n", familyIdxGraphics, queueFlags.c_str());
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxTransfer].queueFlags, queueFlags);
    printf("Vulkan transfer queue family index %d: (%s)\n", familyIdxTransfer, queueFlags.c_str());
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxCompute].queueFlags, queueFlags);
    printf("Vulkan compute queue family index %d:  (%s)\n", familyIdxCompute, queueFlags.c_str());
    RUtil::print_vk_queue_flags(pdevice.familyProps[familyIdxPresent].queueFlags, queueFlags);
    printf("Vulkan present queue family index %d:  (%s)\n", familyIdxPresent, queueFlags.c_str());

    // create a logical device and retrieve queue handles
    std::vector<const char*> desiredDeviceExts{
#ifdef VK_KHR_swapchain
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#endif
    };
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

    vkGetDeviceQueue(self->vk.device, familyIdxPresent, 0, &queueHandle);
    self->vk.queuePresent = create_queue(familyIdxPresent, queueHandle);

    // delegate memory management to VMA
    create_vma_allocator(self);

    // create swapchain
    SwapchainInfo swapchainI;
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

void vk_destroy_device(RDeviceObj* self)
{
    vkDeviceWaitIdle(self->vk.device);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        vk_device_destroy_fence(self, sVulkanFrames[i].frameComplete);
        vk_device_destroy_semaphore(self, sVulkanFrames[i].imageAcquired);
        vk_device_destroy_semaphore(self, sVulkanFrames[i].presentReady);
    }

    destroy_swapchain(self);

    // all VMA allocations should be freed by now
    destroy_vma_allocator(self);

    destroy_queue(self->vk.queuePresent);
    destroy_queue(self->vk.queueCompute);
    destroy_queue(self->vk.queueTransfer);
    destroy_queue(self->vk.queueGraphics);

    vkDestroyDevice(self->vk.device, nullptr);
    vkDestroySurfaceKHR(self->vk.instance, self->vk.surface, nullptr);
    vkDestroyInstance(self->vk.instance, nullptr);

    self->vk.swapchain.~Swapchain();
    self->vk.pdevice.~PhysicalDevice();
}

static RSemaphore vk_device_create_semaphore(RDeviceObj* self, RSemaphoreObj* obj)
{
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
    };
    VK_CHECK(vkCreateSemaphore(self->vk.device, &semaphoreCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_semaphore(RDeviceObj* self, RSemaphore semaphore)
{
    RSemaphoreObj* obj = (RSemaphoreObj*)semaphore;

    vkDestroySemaphore(self->vk.device, obj->vk.handle, nullptr);
}

static RFence vk_device_create_fence(RDeviceObj* self, bool createSignaled, RFenceObj* obj)
{
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlags)0,
    };
    VK_CHECK(vkCreateFence(self->vk.device, &fenceCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_fence(RDeviceObj* self, RFence fence)
{
    RFenceObj* obj = (RFenceObj*)fence;

    vkDestroyFence(self->vk.device, obj->vk.handle, nullptr);
}

static RBuffer vk_device_create_buffer(RDeviceObj* self, const RBufferInfo& bufferI, RBufferObj* obj)
{
    obj->init_vk_api();

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

static void vk_device_destroy_buffer(RDeviceObj* self, RBuffer buffer)
{
    RBufferObj* obj = (RBufferObj*)buffer;

    vmaDestroyBuffer(self->vk.vma, obj->vk.handle, obj->vk.vma);
}

static RImage vk_device_create_image(RDeviceObj* self, const RImageInfo& imageI, RImageObj* obj)
{
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
    } else
        obj->vk.samplerHandle = VK_NULL_HANDLE;

    return {obj};
}

static void vk_device_destroy_image(RDeviceObj* self, RImage image)
{
    RImageObj* obj = (RImageObj*)image;

    if (obj->vk.samplerHandle != VK_NULL_HANDLE)
        vkDestroySampler(self->vk.device, obj->vk.samplerHandle, nullptr);

    vkDestroyImageView(self->vk.device, obj->vk.viewHandle, nullptr);
    vmaDestroyImage(self->vk.vma, obj->vk.handle, obj->vk.vma);
}

// NOTE: the RPass is simplified to contain only a single Vulkan subpass,
//       multiple subpasses may be useful for tiled renderers commonly
//       found in mobile devices, but we keep the render pass API simple for now.
static void vk_device_create_pass(RDeviceObj* self, const RPassInfo& passI, RPassObj* obj)
{
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

static void vk_device_destroy_pass(RDeviceObj* self, RPassObj* passObj)
{
    vkDestroyRenderPass(self->vk.device, passObj->vk.handle, nullptr);
}

static void vk_device_create_framebuffer(RDeviceObj* self, const RFramebufferInfo& fbI, RFramebufferObj* obj)
{
    std::vector<VkImageView> attachments(fbI.colorAttachmentCount);
    for (uint32_t i = 0; i < fbI.colorAttachmentCount; i++)
    {
        RImageObj* imageObj = fbI.colorAttachments[i];
        attachments[i] = imageObj->vk.viewHandle;
    }

    if (fbI.depthStencilAttachment)
    {
        const RImageObj* imageObj = fbI.depthStencilAttachment;
        attachments.push_back(imageObj->vk.viewHandle);
    }

    if (fbI.colorResolveAttachments)
    {
        for (uint32_t i = 0; i < fbI.colorAttachmentCount; i++)
        {
            const RImageObj* imageObj = fbI.colorResolveAttachments[i];
            attachments.push_back(imageObj->vk.viewHandle);
        }
    }

    VkFramebufferCreateInfo fbCI{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = obj->passObj->vk.handle,
        .attachmentCount = (uint32_t)attachments.size(),
        .pAttachments = attachments.data(),
        .width = fbI.width,
        .height = fbI.height,
        .layers = 1,
    };
    VK_CHECK(vkCreateFramebuffer(self->vk.device, &fbCI, nullptr, &obj->vk.handle));
}

static void vk_device_destroy_framebuffer(RDeviceObj* self, RFramebufferObj* obj)
{
    vkDestroyFramebuffer(self->vk.device, obj->vk.handle, nullptr);
}

static RCommandPool vk_device_create_command_pool(RDeviceObj* self, const RCommandPoolInfo& poolI, RCommandPoolObj* obj)
{
    obj->init_vk_api();
    obj->vk.device = self->vk.device;

    VkCommandPoolCreateInfo poolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = self->vk.familyIdxGraphics, // TODO: parameterize against poolI.queueType
    };

    if (poolI.hintTransient)
        poolCI.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VK_CHECK(vkCreateCommandPool(self->vk.device, &poolCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_command_pool(RDeviceObj* self, RCommandPool pool)
{
    RCommandPoolObj* poolObj = (RCommandPoolObj*)pool;

    vkDestroyCommandPool(self->vk.device, poolObj->vk.handle, nullptr);
}

static RShader vk_device_create_shader(RDeviceObj* self, const RShaderInfo& shaderI, RShaderObj* obj)
{
    std::vector<uint32_t> spirvCode;
    RShaderCompiler compiler(self->backend);
    bool success = compiler.compile(shaderI.type, shaderI.glsl, spirvCode);

    if (!success)
        return {};

    VkShaderModuleCreateInfo shaderCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirvCode.size() * 4,
        .pCode = spirvCode.data(),
    };

    VK_CHECK(vkCreateShaderModule(self->vk.device, &shaderCI, nullptr, &obj->vk.handle));

    return {obj};
}

static void vk_device_destroy_shader(RDeviceObj* self, RShader shader)
{
    RShaderObj* shaderObj = (RShaderObj*)shader;

    vkDestroyShaderModule(self->vk.device, shaderObj->vk.handle, nullptr);
}

static RSetPool vk_device_create_set_pool(RDeviceObj* self, const RSetPoolInfo& poolI, RSetPoolObj* poolObj)
{
    poolObj->init_vk_api();
    poolObj->vk.device = self->vk.device;

    std::vector<VkDescriptorPoolSize> poolSizes(poolI.layout.bindingCount);

    for (uint32_t i = 0; i < poolI.layout.bindingCount; i++)
    {
        VkDescriptorType vkType;
        RUtil::cast_binding_type_vk(poolI.layout.bindings[i].type, vkType);
        poolSizes[i].type = vkType;
        poolSizes[i].descriptorCount = poolI.maxSets;
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

static void vk_device_destroy_set_pool(RDeviceObj* self, RSetPool pool)
{
    RSetPoolObj* poolObj = (RSetPoolObj*)pool;

    vkDestroyDescriptorPool(self->vk.device, poolObj->vk.handle, nullptr);
}

void vk_device_create_set_layout(RDeviceObj* self, const RSetLayoutInfo& layoutI, RSetLayoutObj* obj)
{
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

void vk_device_destroy_set_layout(RDeviceObj* self, RSetLayoutObj* layoutObj)
{
    vkDestroyDescriptorSetLayout(self->vk.device, layoutObj->vk.handle, nullptr);
}

void vk_device_create_pipeline_layout(RDeviceObj* self, const RPipelineLayoutInfo& layoutI, RPipelineLayoutObj* layoutObj)
{
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
        setLayoutHandles[i] = layoutObj->setLayoutObjs[i]->vk.handle;

    VkPipelineLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t)setLayoutHandles.size(),
        .pSetLayouts = setLayoutHandles.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range,
    };

    VK_CHECK(vkCreatePipelineLayout(self->vk.device, &layoutCI, nullptr, &layoutObj->vk.handle));
}

static void vk_device_destroy_pipeline_layout(RDeviceObj* self, RPipelineLayoutObj* layoutObj)
{
    vkDestroyPipelineLayout(self->vk.device, layoutObj->vk.handle, nullptr);
}

RPipeline vk_device_create_pipeline(RDeviceObj* self, const RPipelineInfo& pipelineI, RPipelineObj* pipelineObj)
{
    // NOTE: here we only initialize the base pipeline properties,
    //       the actual graphics pipeline is created when variant properties
    //       such as the render pass is known at a later stage.

    pipelineObj->vk.shaderStageCI.resize(pipelineI.shaderCount);

    for (uint32_t i = 0; i < pipelineI.shaderCount; i++)
    {
        RShaderObj* shaderObj = (RShaderObj*)pipelineI.shaders[i];
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

    uint32_t blendAttachmentCount = pipelineI.blend.colorAttachmentCount;
    const RPipelineBlendState* blendStates = pipelineI.blend.colorAttachments;
    pipelineObj->vk.blendStates.resize(blendAttachmentCount);
    for (uint32_t i = 0; i < blendAttachmentCount; i++)
    {
        VkPipelineColorBlendAttachmentState& vkBlendState = pipelineObj->vk.blendStates[i];

        vkBlendState.blendEnable = (VkBool32)blendStates[i].enabled;
        vkBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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

static RPipeline vk_device_create_compute_pipeline(RDeviceObj* self, const RComputePipelineInfo& pipelineI, RPipelineObj* pipelineObj)
{
    const RShaderObj* shaderObj = static_cast<const RShaderObj*>(pipelineI.shader);
    RPipelineLayoutObj* layoutObj = self->get_or_create_pipeline_layout_obj(pipelineI.layout);

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

static void vk_device_destroy_pipeline(RDeviceObj* self, RPipeline pipeline)
{
    RPipelineObj* pipelineObj = (RPipelineObj*)pipeline;

    // destroy all variants
    for (auto& ite : pipelineObj->vk.handles)
        vkDestroyPipeline(self->vk.device, ite.second, nullptr);
}

static void vk_device_pipeline_variant_pass(RDeviceObj* self, RPipelineObj* pipelineObj, const RPassInfo& passI)
{
    RPassObj* passObj = self->get_or_create_pass_obj(passI);
    pipelineObj->vk.variantHash = passObj->hash;

    if (pipelineObj->vk.handles.contains(pipelineObj->vk.variantHash))
        return;

    uint32_t swpWidth = self->vk.swapchain.width;
    uint32_t swpHeight = self->vk.swapchain.height;
    VkViewport viewport = RUtil::make_viewport(swpWidth, swpHeight);
    VkRect2D scissor = RUtil::make_scissor(swpWidth, swpHeight);

    VkPipelineViewportStateCreateInfo viewportSCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkSampleCountFlagBits rasterizationSamples;
    RUtil::cast_sample_count_vk(passI.samples, rasterizationSamples);
    VkPipelineMultisampleStateCreateInfo multisampleSCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = rasterizationSamples,
        .sampleShadingEnable = VK_FALSE,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicSCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (uint32_t)dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (uint32_t)pipelineObj->vk.shaderStageCI.size(),
        .pStages = pipelineObj->vk.shaderStageCI.data(),
        .pVertexInputState = &pipelineObj->vk.vertexInputSCI,
        .pInputAssemblyState = &pipelineObj->vk.inputAsmSCI,
        .pTessellationState = &pipelineObj->vk.tessellationSCI,
        .pViewportState = &viewportSCI,
        .pRasterizationState = &pipelineObj->vk.rasterizationSCI,
        .pMultisampleState = &multisampleSCI,
        .pDepthStencilState = &pipelineObj->vk.depthStencilSCI,
        .pColorBlendState = &pipelineObj->vk.colorBlendSCI,
        .pDynamicState = &dynamicSCI,
        .layout = pipelineObj->layoutObj->vk.handle,
        .renderPass = passObj->vk.handle,
    };

    VkPipeline vkHandle;

    VK_CHECK(vkCreateGraphicsPipelines(self->vk.device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &vkHandle));

    pipelineObj->vk.handles[pipelineObj->vk.variantHash] = vkHandle;
}

static void vk_device_update_set_images(RDeviceObj* self, uint32_t updateCount, const RSetImageUpdateInfo* updates)
{
    std::vector<VkDescriptorImageInfo> imageI;
    std::vector<VkWriteDescriptorSet> writes(updateCount);

    for (uint32_t i = 0; i < updateCount; i++)
    {
        const RSetImageUpdateInfo& update = updates[i];

        for (uint32_t j = 0; j < update.imageCount; j++)
        {
            RImageObj* imageObj = static_cast<RImageObj*>(update.images[j]);
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
        writes[i].dstSet = static_cast<const RSetObj*>(updates[i].set)->vk.handle;
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

static void vk_device_update_set_buffers(RDeviceObj* self, uint32_t updateCount, const RSetBufferUpdateInfo* updates)
{
    std::vector<VkDescriptorBufferInfo> bufferI;
    std::vector<VkWriteDescriptorSet> writes(updateCount);

    for (uint32_t i = 0; i < updateCount; i++)
    {
        const RSetBufferUpdateInfo& update = updates[i];

        for (uint32_t j = 0; j < update.bufferCount; j++)
        {
            RBufferObj* bufferObj = static_cast<RBufferObj*>(update.buffers[j]);

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
        writes[i].dstSet = static_cast<const RSetObj*>(updates[i].set)->vk.handle;
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

static uint32_t vk_device_next_frame(RDeviceObj* self, RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete)
{
    self->vk.frameIdx = (self->vk.frameIdx + 1) % FRAMES_IN_FLIGHT;
    VulkanFrame* frame = sVulkanFrames + self->vk.frameIdx;
    VkSemaphore imageAcquiredSemaphore = static_cast<RSemaphoreObj*>(frame->imageAcquired)->vk.handle;
    VkFence frameCompleteFence = static_cast<RFenceObj*>(frame->frameComplete)->vk.handle;

    VK_CHECK(vkWaitForFences(self->vk.device, 1, &frameCompleteFence, VK_TRUE, UINT64_MAX));

    VkResult result = vkAcquireNextImageKHR(
        self->vk.device,
        self->vk.swapchain.handle,
        UINT64_MAX,
        imageAcquiredSemaphore,
        VK_NULL_HANDLE,
        &self->vk.imageIdx);

    VK_CHECK(result);

    VK_CHECK(vkResetFences(self->vk.device, 1, &frameCompleteFence));

    imageAcquired = frame->imageAcquired;
    presentReady = frame->presentReady;
    frameComplete = frame->frameComplete;

    return self->vk.imageIdx;
}

static void vk_device_present_frame(RDeviceObj* self)
{
    VulkanFrame* frame = sVulkanFrames + self->vk.frameIdx;
    VkSemaphore presentReadySemaphore = static_cast<RSemaphoreObj*>(frame->presentReady)->vk.handle;

    VkPresentInfoKHR presentI{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentReadySemaphore,
        .swapchainCount = 1,
        .pSwapchains = &self->vk.swapchain.handle,
        .pImageIndices = &self->vk.imageIdx,
    };

    VkQueue queueHandle = static_cast<RQueueObj*>(self->vk.queuePresent)->vk.handle;

    // NOTE: this may or may not block, depending on the implementation and
    //       the selected swapchain present mode.
    VK_CHECK(vkQueuePresentKHR(queueHandle, &presentI));
}

static void vk_device_get_depth_stencil_formats(RDeviceObj* self, RFormat* formats, uint32_t& count)
{
    count = (uint32_t)self->vk.pdevice.depthStencilFormats.size();

    if (!formats)
        return;

    for (uint32_t i = 0; i < count; i++)
        RUtil::cast_format_from_vk(self->vk.pdevice.depthStencilFormats[i], formats[i]);
}

static RSampleCountBit vk_device_get_max_sample_count(RDeviceObj* self)
{
    RSampleCountBit sampleCount;
    VkSampleCountFlagBits vkSampleCount = (VkSampleCountFlagBits)self->vk.pdevice.msaaCount;
    RUtil::cast_sample_count_from_vk(vkSampleCount, sampleCount);

    return sampleCount;
}

static RFormat vk_device_get_swapchain_color_format(RDeviceObj* self)
{
    RFormat format;
    RUtil::cast_format_from_vk(self->vk.swapchain.info.imageFormat, format);

    return format;
}

static RImage vk_device_get_swapchain_color_attachment(RDeviceObj* self, uint32_t imageIdx)
{
    return self->vk.swapchain.colorAttachments[imageIdx];
}

static uint32_t vk_device_get_swapchain_image_count(RDeviceObj* self)
{
    return (uint32_t)self->vk.swapchain.images.size();
}

static uint32_t vk_device_get_frames_in_flight_count(RDeviceObj* self)
{
    return FRAMES_IN_FLIGHT;
}

static RQueue vk_device_get_graphics_queue(RDeviceObj* self)
{
    return self->vk.queueGraphics;
}

static void vk_device_wait_idle(RDeviceObj* self)
{
    VK_CHECK(vkDeviceWaitIdle(self->vk.device));
}

static void vk_buffer_map(RBufferObj* self)
{
    RDeviceObj* deviceObj = (RDeviceObj*)self->device;

    VK_CHECK(vmaMapMemory(deviceObj->vk.vma, self->vk.vma, &self->hostMap));
}

static void* vk_buffer_map_read(RBufferObj* self, uint64_t offset, uint64_t size)
{
    char* src = (char*)self->hostMap + offset;

    return (void*)src;
}

static void vk_buffer_map_write(RBufferObj* self, uint64_t offset, uint64_t size, const void* data)
{
    char* dst = (char*)self->hostMap + offset;

    memcpy(dst, data, size);
}

static void vk_buffer_unmap(RBufferObj* self)
{
    RDeviceObj* deviceObj = (RDeviceObj*)self->device;

    vmaUnmapMemory(deviceObj->vk.vma, self->vk.vma);
}

static void vk_command_list_free(RCommandListObj* self)
{
    vkFreeCommandBuffers(self->vk.device, self->poolObj->vk.handle, 1, &self->vk.handle);

    heap_free(self);
}

static void vk_command_list_begin(RCommandListObj* self, bool oneTimeSubmit)
{
    VkCommandBufferBeginInfo beginBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : (VkCommandBufferUsageFlags)0,
        .pInheritanceInfo = nullptr,
    };

    VK_CHECK(vkBeginCommandBuffer(self->vk.handle, &beginBI));
}

static void vk_command_list_end(RCommandListObj* self)
{
    VK_CHECK(vkEndCommandBuffer(self->vk.handle));
}

static void vk_command_list_cmd_begin_pass(RCommandListObj* self, const RPassBeginInfo& passBI)
{
    RFramebufferInfo framebufferI{
        .width = passBI.width,
        .height = passBI.height,
        .colorAttachmentCount = passBI.colorAttachmentCount,
        .colorAttachments = passBI.colorAttachments,
        .colorResolveAttachments = passBI.colorResolveAttachments,
        .depthStencilAttachment = passBI.depthStencilAttachment,
        .pass = passBI.pass,
    };

    RFramebufferObj* framebufferObj = self->deviceObj->get_or_create_framebuffer_obj(framebufferI);

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
        .renderPass = passObj->vk.handle,
        .framebuffer = framebufferObj->vk.handle,
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

static void vk_command_list_cmd_bind_graphics_pipeline(RCommandListObj* self, RPipeline pipeline)
{
    RPipelineObj* pipelineObj = (RPipelineObj*)pipeline;

    LD_ASSERT(pipelineObj->vk.handles.contains(pipelineObj->vk.variantHash));
    VkPipeline vkHandle = pipelineObj->vk.handles[pipelineObj->vk.variantHash];

    vkCmdBindPipeline(self->vk.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vkHandle);
}

static void vk_command_list_cmd_bind_graphics_sets(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t setStart, uint32_t setCount, RSet* sets)
{
    std::vector<VkDescriptorSet> setHandles(setCount);
    for (uint32_t i = 0; i < setCount; i++)
        setHandles[i] = static_cast<RSetObj*>(sets[i])->vk.handle;

    vkCmdBindDescriptorSets(self->vk.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutObj->vk.handle, setStart, setCount, setHandles.data(), 0, nullptr);
}

static void vk_command_list_cmd_bind_compute_pipeline(RCommandListObj* self, RPipeline pipeline)
{
    RPipelineObj* pipelineObj = (RPipelineObj*)pipeline;

    LD_ASSERT(pipelineObj->vk.handles.contains(0));
    VkPipeline vkHandle = pipelineObj->vk.handles[0];

    vkCmdBindPipeline(self->vk.handle, VK_PIPELINE_BIND_POINT_COMPUTE, vkHandle);
}

static void vk_command_list_cmd_bind_compute_sets(RCommandListObj* self, RPipelineLayoutObj* layoutObj, uint32_t setStart, uint32_t setCount, RSet* sets)
{
    std::vector<VkDescriptorSet> setHandles(setCount);
    for (uint32_t i = 0; i < setCount; i++)
        setHandles[i] = static_cast<RSetObj*>(sets[i])->vk.handle;

    vkCmdBindDescriptorSets(self->vk.handle, VK_PIPELINE_BIND_POINT_COMPUTE, layoutObj->vk.handle, setStart, setCount, setHandles.data(), 0, nullptr);
}

static void vk_command_list_cmd_bind_vertex_buffers(RCommandListObj* self, uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers)
{
    std::vector<VkBuffer> bufferHandles(bindingCount);
    std::vector<VkDeviceSize> bufferOffsets(bindingCount);

    for (uint32_t i = 0; i < bindingCount; i++)
    {
        bufferHandles[i] = static_cast<RBufferObj*>(buffers[i])->vk.handle;
        bufferOffsets[i] = 0;
    }

    vkCmdBindVertexBuffers(self->vk.handle, firstBinding, bindingCount, bufferHandles.data(), bufferOffsets.data());
}

static void vk_command_list_cmd_bind_index_buffer(RCommandListObj* self, RBuffer buffer, RIndexType indexType)
{
    VkBuffer bufferHandle = static_cast<RBufferObj*>(buffer)->vk.handle;
    VkIndexType vkIndexType;

    RUtil::cast_index_type_vk(indexType, vkIndexType);

    vkCmdBindIndexBuffer(self->vk.handle, bufferHandle, 0, vkIndexType);
}

static void vk_command_list_cmd_dispatch(RCommandListObj* self, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDispatch(self->vk.handle, groupCountX, groupCountY, groupCountZ);
}

static void vk_command_list_cmd_draw(RCommandListObj* self, const RDrawInfo& drawI)
{
    vkCmdDraw(self->vk.handle, drawI.vertexCount, drawI.instanceCount, drawI.vertexStart, drawI.instanceStart);
}

static void vk_command_list_cmd_draw_indexed(RCommandListObj* self, const RDrawIndexedInfo& drawI)
{
    vkCmdDrawIndexed(self->vk.handle, drawI.indexCount, drawI.instanceCount, drawI.indexStart, 0, drawI.instanceStart);
}

static void vk_command_list_cmd_end_pass(RCommandListObj* self)
{
    vkCmdEndRenderPass(self->vk.handle);
}

static void vk_command_list_cmd_buffer_memory_barrier(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier)
{
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
        .buffer = static_cast<const RBufferObj*>(barrier.buffer)->vk.handle,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    vkCmdPipelineBarrier(self->vk.handle, vkSrcStages, vkDstStages, 0, 0, nullptr, 1, &vkBarrier, 0, nullptr);
}

static void vk_command_list_cmd_image_memory_barrier(RCommandListObj* self, RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier)
{
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
        .image = static_cast<const RImageObj*>(barrier.image)->vk.handle,
        .subresourceRange = range,
    };

    vkCmdPipelineBarrier(self->vk.handle, vkSrcStages, vkDstStages, 0, 0, nullptr, 0, nullptr, 1, &vkBarrier);
}

static void vk_command_list_cmd_copy_buffer(RCommandListObj* self, RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions)
{
    VkBuffer srcBufferHandle = static_cast<RBufferObj*>(srcBuffer)->vk.handle;
    VkBuffer dstBufferHandle = static_cast<RBufferObj*>(dstBuffer)->vk.handle;

    std::vector<VkBufferCopy> copies(regionCount);
    for (uint32_t i = 0; i < regionCount; i++)
    {
        copies[i].srcOffset = regions[i].srcOffset;
        copies[i].dstOffset = regions[i].dstOffset;
        copies[i].size = regions[i].size;
    }

    vkCmdCopyBuffer(self->vk.handle, srcBufferHandle, dstBufferHandle, regionCount, copies.data());
}

static void vk_command_list_cmd_copy_buffer_to_image(RCommandListObj* self, RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions)
{
    VkBuffer srcBufferHandle = static_cast<RBufferObj*>(srcBuffer)->vk.handle;
    VkImage dstImageHandle = static_cast<RImageObj*>(dstImage)->vk.handle;
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

static void vk_command_list_cmd_copy_image_to_buffer(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions)
{
    VkBuffer dstBufferHandle = static_cast<RBufferObj*>(dstBuffer)->vk.handle;
    VkImage srcImageHandle = static_cast<RImageObj*>(srcImage)->vk.handle;
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

static void vk_command_list_cmd_blit_image(RCommandListObj* self, RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter)
{
    RImageObj* srcImageObj = (RImageObj*)srcImage;
    RImageObj* dstImageObj = (RImageObj*)dstImage;

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

static RCommandList vk_command_pool_allocate(RCommandPoolObj* self, RCommandListObj* listObj)
{
    VkCommandBufferAllocateInfo bufferAI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = self->vk.handle,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    listObj->init_vk_api();
    listObj->vk.device = self->vk.device;

    VK_CHECK(vkAllocateCommandBuffers(self->vk.device, &bufferAI, &listObj->vk.handle));

    return {listObj};
}

static void vk_command_pool_reset(RCommandPoolObj* self)
{
    VK_CHECK(vkResetCommandPool(self->vk.device, self->vk.handle, 0));
}

static RSet vk_set_pool_allocate(RSetPoolObj* self, RSetObj* setObj)
{
    VkDescriptorSetAllocateInfo setAI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = self->vk.handle,
        .descriptorSetCount = 1,
        .pSetLayouts = &self->layoutObj->vk.handle,
    };

    VK_CHECK(vkAllocateDescriptorSets(self->vk.device, &setAI, &setObj->vk.handle));

    return {setObj};
}

static void vk_set_pool_reset(RSetPoolObj* self)
{
    VK_CHECK(vkResetDescriptorPool(self->vk.device, self->vk.handle, 0));
}

static void vk_queue_wait_idle(RQueueObj* self)
{
    VK_CHECK(vkQueueWaitIdle(self->vk.handle));
}

static void vk_queue_submit(RQueueObj* self, const RSubmitInfo& submitI, RFence fence)
{
    VkFence fenceHandle = fence ? static_cast<RFenceObj*>(fence)->vk.handle : VK_NULL_HANDLE;

    std::vector<VkSemaphore> semaphoreHandles(submitI.waitCount + submitI.signalCount);

    uint32_t i;

    for (i = 0; i < submitI.waitCount; i++)
        semaphoreHandles[i] = static_cast<RSemaphoreObj*>(submitI.waits[i])->vk.handle;

    for (i = 0; i < submitI.signalCount; i++)
        semaphoreHandles[submitI.waitCount + i] = static_cast<RSemaphoreObj*>(submitI.signals[i])->vk.handle;

    std::vector<VkCommandBuffer> commandHandles(submitI.listCount);

    for (i = 0; i < submitI.listCount; i++)
        commandHandles[i] = static_cast<RCommandListObj*>(submitI.lists[i])->vk.handle;

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

static void choose_physical_device(RDeviceObj* obj)
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
        printf("VkPhysicalDevice: %s\n", pdevice.deviceProps.deviceName);

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

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle, obj->vk.surface, &pdevice.surfaceCaps));

        // available surface formats on this physical device
        uint32_t formatCount;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(handle, obj->vk.surface, &formatCount, nullptr));
        pdevice.surfaceFormats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(handle, obj->vk.surface, &formatCount, pdevice.surfaceFormats.data()));

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
        uint32_t modeCount;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(handle, obj->vk.surface, &modeCount, NULL));
        pdevice.presentModes.resize(modeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(handle, obj->vk.surface, &modeCount, pdevice.presentModes.data()));

        if (chosen)
            break;
    }
}

static void configure_swapchain(RDeviceObj* obj, SwapchainInfo* swapchainI)
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
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // vsynced
            swapchainI->presentMode = mode;
    }
}

static void create_swapchain(RDeviceObj* obj, const SwapchainInfo& swapchainI)
{
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

    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = obj->vk.surface,
        .minImageCount = minImageCount,
        .imageFormat = swapchainI.imageFormat,
        .imageColorSpace = swapchainI.imageColorSpace,
        .imageExtent = pdevice.surfaceCaps.currentExtent,
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
    } else
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

    printf("Vulkan swapchain with %d images (hint %d, min %d, max %d)\n", (int)swp.images.size(),
           (int)swapchainImageHint, (int)surfaceMinImageCount, (int)surfaceMaxImageCount);

    std::string presentMode;
    RUtil::print_vk_present_mode(swp.info.presentMode, presentMode);
    printf("Vulkan swapchain present mode  %s\n", presentMode.c_str());
}

static RImage create_swapchain_color_attachment(RDeviceObj* deviceObj, VkImage image, VkFormat colorFormat, uint32_t width, uint32_t height)
{
    RImageObj* obj = (RImageObj*)heap_malloc(sizeof(RImageObj), MEMORY_USAGE_RENDER);
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

static void destroy_swapchain(RDeviceObj* obj)
{
    for (RImage attachment : obj->vk.swapchain.colorAttachments)
        destroy_swapchain_color_attachment(obj, attachment);

    vkDestroySwapchainKHR(obj->vk.device, obj->vk.swapchain.handle, nullptr);
}

static void destroy_swapchain_color_attachment(RDeviceObj* deviceObj, RImage attachment)
{
    RImageObj* obj = (RImageObj*)attachment;

    vkDestroyImageView(deviceObj->vk.device, obj->vk.viewHandle, nullptr);

    heap_free(obj);
}

static void create_vma_allocator(RDeviceObj* obj)
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

static void destroy_vma_allocator(RDeviceObj* obj)
{
    vmaDestroyAllocator(obj->vk.vma);
}

static RQueue create_queue(uint32_t queueFamilyIdx, VkQueue handle)
{
    RQueueObj* obj = (RQueueObj*)heap_malloc(sizeof(RQueueObj), MEMORY_USAGE_RENDER);
    obj->init_vk_api();
    obj->vk.familyIdx = queueFamilyIdx;
    obj->vk.handle = handle;

    return {obj};
}

static void destroy_queue(RQueue queue)
{
    RQueueObj* obj = (RQueueObj*)queue;

    heap_free(obj);
}

void RBufferObj::init_vk_api()
{
    map = &vk_buffer_map;
    map_read = &vk_buffer_map_read;
    map_write = &vk_buffer_map_write;
    unmap = &vk_buffer_unmap;
}

void RCommandListObj::init_vk_api()
{
    begin = &vk_command_list_begin;
    end = &vk_command_list_end;
    cmd_begin_pass = &vk_command_list_cmd_begin_pass;
    cmd_bind_graphics_pipeline = &vk_command_list_cmd_bind_graphics_pipeline;
    cmd_bind_graphics_sets = &vk_command_list_cmd_bind_graphics_sets;
    cmd_bind_compute_pipeline = &vk_command_list_cmd_bind_compute_pipeline;
    cmd_bind_compute_sets = &vk_command_list_cmd_bind_compute_sets;
    cmd_bind_vertex_buffers = &vk_command_list_cmd_bind_vertex_buffers;
    cmd_bind_index_buffer = &vk_command_list_cmd_bind_index_buffer;
    cmd_dispatch = &vk_command_list_cmd_dispatch;
    cmd_draw = &vk_command_list_cmd_draw;
    cmd_draw_indexed = &vk_command_list_cmd_draw_indexed;
    cmd_end_pass = &vk_command_list_cmd_end_pass;
    cmd_buffer_memory_barrier = &vk_command_list_cmd_buffer_memory_barrier;
    cmd_image_memory_barrier = &vk_command_list_cmd_image_memory_barrier;
    cmd_copy_buffer = &vk_command_list_cmd_copy_buffer;
    cmd_copy_buffer_to_image = &vk_command_list_cmd_copy_buffer_to_image;
    cmd_copy_image_to_buffer = &vk_command_list_cmd_copy_image_to_buffer;
    cmd_blit_image = &vk_command_list_cmd_blit_image;
}

void RCommandPoolObj::init_vk_api()
{
    allocate = &vk_command_pool_allocate;
    reset = &vk_command_pool_reset;
}

void RSetPoolObj::init_vk_api()
{
    allocate = &vk_set_pool_allocate;
    reset = &vk_set_pool_reset;
}

void RQueueObj::init_vk_api()
{
    wait_idle = &vk_queue_wait_idle;
    submit = &vk_queue_submit;
}

void RDeviceObj::init_vk_api()
{
    create_fence = &vk_device_create_fence;
    destroy_fence = &vk_device_destroy_fence;
    create_semaphore = &vk_device_create_semaphore;
    destroy_semaphore = &vk_device_destroy_semaphore;
    create_buffer = &vk_device_create_buffer;
    destroy_buffer = &vk_device_destroy_buffer;
    create_image = &vk_device_create_image;
    destroy_image = &vk_device_destroy_image;
    create_pass = &vk_device_create_pass;
    destroy_pass = &vk_device_destroy_pass;
    create_framebuffer = &vk_device_create_framebuffer;
    destroy_framebuffer = &vk_device_destroy_framebuffer;
    create_command_pool = &vk_device_create_command_pool;
    destroy_command_pool = &vk_device_destroy_command_pool;
    create_shader = &vk_device_create_shader;
    destroy_shader = &vk_device_destroy_shader;
    create_set_pool = &vk_device_create_set_pool;
    destroy_set_pool = &vk_device_destroy_set_pool;
    create_set_layout = &vk_device_create_set_layout;
    destroy_set_layout = &vk_device_destroy_set_layout;
    create_pipeline_layout = &vk_device_create_pipeline_layout;
    destroy_pipeline_layout = &vk_device_destroy_pipeline_layout;
    create_pipeline = &vk_device_create_pipeline;
    create_compute_pipeline = &vk_device_create_compute_pipeline;
    destroy_pipeline = &vk_device_destroy_pipeline;
    pipeline_variant_pass = &vk_device_pipeline_variant_pass;
    update_set_images = &vk_device_update_set_images;
    update_set_buffers = &vk_device_update_set_buffers;
    next_frame = &vk_device_next_frame;
    present_frame = &vk_device_present_frame;
    get_depth_stencil_formats = &vk_device_get_depth_stencil_formats;
    get_max_sample_count = &vk_device_get_max_sample_count;
    get_swapchain_color_format = &vk_device_get_swapchain_color_format;
    get_swapchain_color_attachment = &vk_device_get_swapchain_color_attachment;
    get_swapchain_image_count = &vk_device_get_swapchain_image_count;
    get_frames_in_flight_count = &vk_device_get_frames_in_flight_count;
    get_graphics_queue = &vk_device_get_graphics_queue;
    wait_idle = &vk_device_wait_idle;
}

} // namespace LD
