#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackendEnum.h>
#include <cstdint>

struct GLFWwindow;

namespace LD {

template <typename TObject>
class RHandle
{
public:
    RHandle() = default;
    RHandle(TObject* obj) : mObj(obj) {}

    /// @brief get an id unique to each RHandle
    /// @warning does not check for null handle before derefencing
    inline uint64_t rid() const { return *(uint64_t*)mObj; }
    inline TObject* unwrap() { return mObj; }
    inline const TObject* unwrap() const { return mObj; }
    inline operator bool() const { return mObj != nullptr; }

    /// @brief two handles are equal if they reference the same object
    /// @return true if both handles are not null and reference the same object.
    bool operator==(const RHandle& other) const { return mObj && other.mObj && (*(uint64_t*)mObj == *(uint64_t*)other.mObj); }
    bool operator!=(const RHandle& other) const { return !operator==(other); }

protected:
    TObject* mObj;
};

/// @brief semaphore handle, used in GPU-GPU synchronization
struct RSemaphore : RHandle<struct RSemaphoreObj>
{
};

/// @brief fence handle, used in CPU-GPU synchronization
struct RFence : RHandle<struct RFenceObj>
{
};

/// @brief describes a buffer copy region
struct RBufferCopy
{
    uint64_t srcOffset;
    uint64_t dstOffset;
    uint64_t size;
};

/// @brief describes a copy region between a buffer and an image
struct RBufferImageCopy
{
    uint64_t bufferOffset;
    uint32_t imageWidth;
    uint32_t imageHeight;
    uint32_t imageDepth;
    uint32_t imageLayers;
};

/// @brief describes a copy region between images in a blit operation
struct RImageBlit
{
    struct
    {
        uint32_t x;
        uint32_t y;
        uint32_t z;
    } srcMinOffset, srcMaxOffset, dstMinOffset, dstMaxOffset;
};

/// @brief renderer buffer creation info
struct RBufferInfo
{
    RBufferUsageFlags usage;
    uint64_t size;
    bool hostVisible;
};

/// @brief renderer buffer handle
struct RBuffer : RHandle<struct RBufferObj>
{
    /// @brief byte size of the buffer
    uint64_t size() const;

    /// @brief usages of the buffer
    RBufferUsageFlags usage() const;

    void map();
    void* map_read(uint32_t offset, uint64_t size);
    void map_write(uint64_t offset, uint64_t size, const void* data);
    void unmap();
};

struct RSamplerInfo
{
    RFilter filter;
    RFilter mipmapFilter;
    RSamplerAddressMode addressMode;
};

/// @brief renderer image creation info
struct RImageInfo
{
    RImageUsageFlags usage;
    RImageType type;
    RSampleCountBit samples;
    RFormat format;
    uint32_t layers;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    RSamplerInfo sampler; /// if usage contains RIMAGE_USAGE_SAMPLED_BIT, this describes the sampler
};

/// @brief renderer image handle
struct RImage : RHandle<struct RImageObj>
{
    /// @brief the usage of the image
    RImageUsageFlags usage() const;

    /// @brief the type of the image
    RImageType type() const;

    /// @brief the format of the image
    RFormat format() const;

    /// @brief the width of the image
    uint32_t width() const;

    /// @brief the height of the image
    uint32_t height() const;

    /// @brief the depth of the image
    uint32_t depth() const;

    /// @brief number of layers in the image
    uint32_t layers() const;

    /// @brief inferred byte size of mipmap level 0 from image format, width, height, depth, and layers
    uint64_t size() const;
};

/// @brief description of how a color attachment is used in a render pass
struct RPassColorAttachment
{
    RFormat colorFormat;
    RAttachmentLoadOp colorLoadOp;
    RAttachmentStoreOp colorStoreOp;
    RImageLayout initialLayout; /// the color layout after previous render pass, or RIMAGE_LAYOUT_UNDEFINED
    RImageLayout passLayout;    /// the color layout to transition to when the render pass begins
};

/// @brief description of how a depth stencil attachment is used in a render pass
struct RPassDepthStencilAttachment
{
    RFormat depthStencilFormat;
    RAttachmentLoadOp depthLoadOp;
    RAttachmentStoreOp depthStoreOp;
    RAttachmentLoadOp stencilLoadOp;
    RAttachmentStoreOp stencilStoreOp;
    RImageLayout initialLayout; /// the depth stencil layout after previous render pass, or RIMAGE_LAYOUT_UNDEFINED
    RImageLayout passLayout;    /// the depth stencil layout to transition to when the render pass begins
};

/// @brief description of how a resolve attachment is used in a render pass,
///        while the image format is not specified here, it is expected to
///        be identical to the corresponding color/depth multisampled attachment.
struct RPassResolveAttachment
{
    RAttachmentLoadOp loadOp;   /// how the color/depth contents are treated when the render pass begins
    RAttachmentStoreOp storeOp; /// how the color/depth contents are treated when the render pass ends
    RImageLayout initialLayout; /// the resolve attachment layout before the render pass, or RIMAGE_LAYOUT_UNDEFINED
    RImageLayout passLayout;    /// the resolve attachment layout to transition to when the render pass begins
};

struct RPassDependency
{
    RPipelineStageFlags srcStageMask;
    RPipelineStageFlags dstStageMask;
    RAccessFlags srcAccessMask;
    RAccessFlags dstAccessMask;
};

/// @brief render pass creation info
struct RPassInfo
{
    /// if not equal to RSAMPLE_COUNT_1_BIT, implies that all colorAttachments (and depthStencilAttachment if not null)
    /// are multisampled, and the color attachments are resolved with colorResolveAttachments
    RSampleCountBit samples;

    uint32_t colorAttachmentCount;

    const RPassColorAttachment* colorAttachments;

    /// if not null, an array of colorAttachmentCount resolve attachments,
    /// and colorAttachments is expected to be an array of multisampled images
    const RPassResolveAttachment* colorResolveAttachments;

    /// if not null, the depth stencil attachment used for depth and stencil tests
    const RPassDepthStencilAttachment* depthStencilAttachment;

    /// render pass dependency protects the attachments and transitions the image layouts,
    /// comparable to an image memory barrier.
    const RPassDependency* dependency;
};

union RClearColorValue
{
    float float32[4];
    int32_t int32[4];
    uint32_t uint32[4];
};

struct RClearDepthStencilValue
{
    float depth;
    uint32_t stencil;
};

/// @brief render pass instance creation info, used during command list recording
struct RPassBeginInfo
{
    uint32_t width;                  /// render area width
    uint32_t height;                 /// render area height
    RImage depthStencilAttachment;   /// if not a null handle, the depth stencil attachment for this pass
    uint32_t colorAttachmentCount;   /// number of color attachments used in this render pass
    RImage* colorAttachments;        /// an array of valid image handles
    RImage* colorResolveAttachments; /// if not null, an array of colorAttachmentCount resolve attachments for colorAttachments

    /// @brief if the i'th color attachment in this pass uses RATTACHMENT_LOAD_OP_CLEAR,
    ///        clearColors[i] will be used to clear the attachment when the pass begins.
    RClearColorValue* clearColors;

    /// @brief if depthStencilAttachment is not a null handle and uses RATTACHMENT_LOAD_OP_CLEAR,
    //         this value is used to clear the attachment when the pass begins.
    RClearDepthStencilValue clearDepthStencil;

    RPassInfo pass; /// render pass description
};

/// @brief shader module creation info
struct RShaderInfo
{
    RShaderType type; /// shader module type
    const char* glsl; /// Vulkan glsl source code string, null terminated
};

/// @brief shader handle
struct RShader : RHandle<struct RShaderObj>
{
};

/// @brief describes a resource binding within a resource set
struct RSetBindingInfo
{
    uint32_t binding;    /// the index of this binding within the set
    RBindingType type;   /// the type of this binding
    uint32_t arrayCount; /// if greater than one, the binding array size
};

/// @brief resource set layout info
struct RSetLayoutInfo
{
    uint32_t bindingCount;
    RSetBindingInfo* bindings;
};

/// @brief resource set handle
struct RSet : RHandle<struct RSetObj>
{
};

/// @brief resource set pool creation info
struct RSetPoolInfo
{
    RSetLayoutInfo layout;
    uint32_t maxSets;
};

/// @brief resource set pool handle, used to allocate resource sets
struct RSetPool : RHandle<struct RSetPoolObj>
{
    /// @brief allocate a resource set
    RSet allocate();

    /// @brief returns all allocated set to the pool
    /// @warning all set handles previously allocated will become out of scope
    void reset();
};

struct RPipelineLayoutInfo
{
    uint32_t setLayoutCount;    /// number of sets the pipeline layout
    RSetLayoutInfo* setLayouts; /// layout of each set, starting at index zero
};

struct RVertexAttribute
{
    RGLSLType type;   // vertex attribute glsl data type
    uint32_t offset;  // offset from start of vertex
    uint32_t binding; // corresponding RVertexBinding
};

struct RVertexBinding
{
    RBindingInputRate inputRate; // attribute input rate
    uint32_t stride;             // vertex stride
};

/// @brief graphics pipeline rasterization state info
struct RPipelineRasterizationInfo
{
    RPolygonMode polygonMode;
    RCullMode cullMode;
    float lineWidth = 1.0f; /// used for RPOLYGON_MODE_LINE
};

/// @brief graphics pipeline depth stencil state info
struct RPipelineDepthStencilInfo
{
    bool depthTestEnabled;
    bool depthWriteEnabled;
    RCompareOp depthCompareOp;
};

/// @brief describes the blend state of a color attachment
struct RPipelineBlendState
{
    bool enabled;
    RBlendFactor srcColorFactor;
    RBlendFactor dstColorFactor;
    RBlendFactor srcAlphaFactor;
    RBlendFactor dstAlphaFactor;
    RBlendOp colorBlendOp;
    RBlendOp alphaBlendOp;
};

/// @brief graphics pipeline blend state info
struct RPipelineBlendInfo
{
    uint32_t colorAttachmentCount = 0; /// disable blending for all attachments if zero
    RPipelineBlendState* colorAttachments;
};

/// @brief graphics pipeline creation info
struct RPipelineInfo
{
    uint32_t shaderCount;
    RShader* shaders;
    uint32_t vertexAttributeCount;
    RVertexAttribute* vertexAttributes;
    uint32_t vertexBindingCount;
    RVertexBinding* vertexBindings;
    RPrimitiveTopology primitiveTopology;
    RPipelineLayoutInfo layout;
    RPipelineRasterizationInfo rasterization;
    RPipelineDepthStencilInfo depthStencil;
    RPipelineBlendInfo blend;
};

/// @brief compute pipeline creation info
struct RComputePipelineInfo
{
    RPipelineLayoutInfo layout;
    RShader shader;
};

/// @brief graphics pipeline handle
struct RPipeline : RHandle<struct RPipelineObj>
{
    /// @brief specify color write masks for a color attachment
    /// @param index color attachment index
    /// @param mask color channels that are allowed to write
    void set_color_write_mask(uint32_t index, RColorComponentFlags mask);

    /// @brief specify whether depth testing is enabled
    void set_depth_test_enable(bool enable);
};

/// @brief vertex draw call information
struct RDrawInfo
{
    uint32_t vertexCount;
    uint32_t vertexStart; /// the starting gl_VertexIndex
    uint32_t instanceCount;
    uint32_t instanceStart; /// the starting gl_InstanceIndex
};

/// @brief indexed draw call information
struct RDrawIndexedInfo
{
    uint32_t indexCount;
    uint32_t indexStart; /// first index is sourced from IndexBuffer[indexStart]
    uint32_t instanceCount;
    uint32_t instanceStart; /// the starting gl_InstanceIndex
};

struct RBufferMemoryBarrier
{
    RBuffer buffer;
    RAccessFlags srcAccess;
    RAccessFlags dstAccess;
};

struct RImageMemoryBarrier
{
    RImage image;
    RImageLayout oldLayout;
    RImageLayout newLayout;
    RAccessFlags srcAccess;
    RAccessFlags dstAccess;
};

/// @brief command list handle
struct RCommandList : RHandle<struct RCommandListObj>
{
    void begin();

    void end();

    void reset();

    /// @brief begin a render pass instance
    void cmd_begin_pass(const RPassBeginInfo& passBI);

    /// @brief update push constants
    void cmd_push_constant(const RPipelineLayoutInfo& layout, uint32_t offset, uint32_t size, const void* data);

    void cmd_bind_graphics_pipeline(RPipeline pipeline);

    void cmd_bind_graphics_sets(const RPipelineLayoutInfo& layout, uint32_t firstSet, uint32_t setCount, RSet* sets);

    void cmd_bind_compute_pipeline(RPipeline pipeline);

    void cmd_bind_compute_sets(const RPipelineLayoutInfo& layout, uint32_t firstSet, uint32_t setCount, RSet* sets);

    void cmd_bind_vertex_buffers(uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);

    void cmd_bind_index_buffer(RBuffer buffer, RIndexType indexType);

    /// @brief dispatch compute workgroup
    void cmd_dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    /// @brief set scissor rect for subsequent draw calls
    void cmd_set_scissor(const Rect& scissor);

    /// @brief draw vertices
    void cmd_draw(const RDrawInfo& drawI);

    /// @brief indexed draw call
    void cmd_draw_indexed(const RDrawIndexedInfo& drawI);

    /// @brief end the current render pass instance
    void cmd_end_pass();

    /// @brief add a buffer memory barrier
    void cmd_buffer_memory_barrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RBufferMemoryBarrier& barrier);

    /// @brief add an image memory barrier
    void cmd_image_memory_barrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);

    /// @brief a transfer command to copy from buffer to buffer
    void cmd_copy_buffer(RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);

    /// @brief a transfer command to copy from buffer to image
    void cmd_copy_buffer_to_image(RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);

    /// @brief a transfer command to copy from image to buffer
    void cmd_copy_image_to_buffer(RImage srcImage, RImageLayout srcImageLayout, RBuffer dstBuffer, uint32_t regionCount, const RBufferImageCopy* regions);

    /// @brief a transfer command to copy between images, potentially performing format conversion
    void cmd_blit_image(RImage srcImage, RImageLayout srcImageLayout, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RImageBlit* regions, RFilter filter);
};

/// @brief command pool creation info
struct RCommandPoolInfo
{
    RQueueType queueType;
    bool hintTransient;  /// hint to the implementation that command lists allocated from this pool will be short lived
    bool listResettable; /// whether or not command lists allocated from this pool can be reset individually.
};

/// @brief command pool handle, used to allocate command lists
struct RCommandPool : RHandle<struct RCommandPoolObj>
{
    /// @brief allocate a command list
    RCommandList allocate();

    void reset();
};

/// @brief describes the workload to send to GPU
struct RSubmitInfo
{
    uint32_t waitCount; /// number of semaphores to wait before any of the command lists begin execution
    RPipelineStageFlags* waitStages;
    RSemaphore* waits;
    uint32_t listCount; /// number of command lists to submit
    RCommandList* lists;
    uint32_t signalCount; /// number of semaphores to signal after all command lists complete execution
    RSemaphore* signals;
};

/// @brief queue handle, all GPU work are submitted through a queue
struct RQueue : RHandle<struct RQueueObj>
{
    /// @brief blocks until all work on this queue is complete
    void wait_idle();

    /// @brief submits work to this queue
    /// @param submitI submission info
    /// @param fence if not null, will be signaled after all command lists complete execution
    void submit(const RSubmitInfo& submitI, RFence fence);
};

struct RSetImageUpdateInfo
{
    RSet set;                      /// the resource set to update
    uint32_t dstBinding;           /// the binding within the set
    uint32_t dstArrayIndex;        /// the starting array index of the binding
    uint32_t imageCount;           /// number of image bindings to update
    RBindingType imageBindingType; /// binding type
    RImage* images;                /// array of image handles
    RImageLayout* imageLayouts;    /// array of current image layouts
};

struct RSetBufferUpdateInfo
{
    RSet set; /// the resource set to update
    uint32_t dstBinding;
    uint32_t dstArrayIndex;
    uint32_t bufferCount;
    RBindingType bufferBindingType; /// binding type of the buffer
    RBuffer* buffers;
};

/// @brief render device creation info
struct RDeviceInfo
{
    RDeviceBackend backend;
    GLFWwindow* window;
    bool vsync;
};

/// @brief render device handle
struct RDevice : RHandle<struct RDeviceObj>
{
    static RDevice create(const RDeviceInfo& deviceI);
    static void destroy(RDevice device);

    RSemaphore create_semaphore();
    void destroy_semaphore(RSemaphore semaphore);

    RFence create_fence(bool createSignaled);
    void destroy_fence(RFence fence);

    RBuffer create_buffer(const RBufferInfo& bufferI);
    void destroy_buffer(RBuffer buffer);

    RImage create_image(const RImageInfo& imageI);
    void destroy_image(RImage image);

    RCommandPool create_command_pool(const RCommandPoolInfo& poolI);
    void destroy_command_pool(RCommandPool pool);

    RShader create_shader(const RShaderInfo& shaderI);
    void destroy_shader(RShader shader);

    RSetPool create_set_pool(const RSetPoolInfo& poolI);
    void destroy_set_pool(RSetPool pool);

    RPipeline create_pipeline(const RPipelineInfo& pipelineI);
    RPipeline create_compute_pipeline(const RComputePipelineInfo& pipelineI);
    void destroy_pipeline(RPipeline);

    void update_set_images(uint32_t updateCount, const RSetImageUpdateInfo* updates);

    void update_set_buffers(uint32_t updateCount, const RSetBufferUpdateInfo* updates);

    /// @brief The most important function of the render device, defines the
    ///        GPU frame boundaries. Blocks until the frame-complete fence of
    ///        the corresponding frame is signaled.
    /// @return an index used to retrieve swapchain resources for this frame
    /// @param imageAcquired user waits for this semaphore before rendering to the swapchain color attachment
    /// @param presentReady user signals this semaphore to indicate that the swapchain color attachment is ready for presentation
    /// @param frameComplete user signals this fence to indicate that the frame is complete, synchronizing CPU-GPU frame boundaries
    /// @warning the returned indices each frame are not guaranteed to form a cyclic sequence.
    uint32_t next_frame(RSemaphore& imageAcquired, RSemaphore& presentReady, RFence& frameComplete);

    /// @brief waits until presentReady semaphore is signaled and blocks until presentation is complete
    void present_frame();

    void get_depth_stencil_formats(RFormat* formats, uint32_t& count);

    /// @brief get maximum multisample bits supported by both color and depth attachments,
    ///        if RSAMPLE_COUNT_1_BIT is returned then MSAA is not supported.
    RSampleCountBit get_max_sample_count();

    RFormat get_swapchain_color_format();

    RImage get_swapchain_color_attachment(uint32_t imageIdx);

    uint32_t get_swapchain_image_count();

    void get_swapchain_extent(uint32_t* width, uint32_t* height);

    uint32_t get_frames_in_flight_count();

    /// @brief get a frame index in the half open range [0, frames_in_flight_count)
    uint32_t get_frame_index();

    RQueue get_graphics_queue();

    void wait_idle();
};

/// @brief get a 32 bit hash of render pass
uint32_t hash32_pass_info(const RPassInfo& passI);

/// @brief get a 32 bit hash of resource set layout
uint32_t hash32_set_layout_info(const RSetLayoutInfo& layoutI);

/// @brief get a 32 bit hash of pipeline layout
uint32_t hash32_pipeline_layout_info(const RPipelineLayoutInfo& layoutI);

/// @brief get a 32 bit hash of pipeline rasterization state
uint32_t hash32_pipeline_rasterization_state(const RPipelineRasterizationInfo& ratserizationI);

} // namespace LD
