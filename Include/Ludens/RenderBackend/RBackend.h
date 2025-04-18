#pragma once

#include <Ludens/RenderBackend/RBackendEnum.h>
#include <cstdint>

struct GLFWwindow;

namespace LD {

template <typename TObject>
class RHandle
{
public:
    RHandle() : mObj(nullptr) {}
    RHandle(TObject* obj) : mObj(obj) {}

    operator bool() const { return mObj != nullptr; }
    operator TObject*() { return mObj; }
    operator const TObject*() const { return mObj; }

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
    RFormat format;
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
    RImageLayout finalLayout;   /// the color layout to transition to after the render pass ends
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
    RImageLayout finalLayout;   /// the depth stencil layout to transition to when after render pass ends
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
    uint32_t colorAttachmentCount;
    RPassColorAttachment* colorAttachments;
    RPassDepthStencilAttachment* depthStencilAttachment;
    RPassDependency* srcDependency; /// if not null, describes how we depend on the previous (src) render pass
    RPassDependency* dstDependency; /// if not null, describes how the next (dst) render pass depends on us
};

/// @brief render pass handle
struct RPass : RHandle<struct RPassObj>
{
    uint32_t hash() const;

    uint32_t color_attachment_count() const;
    
    bool has_depth_stencil_attachment() const;
};

/// @brief framebuffer creation info
struct RFramebufferInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t colorAttachmentCount;
    RImage* colorAttachments;
    RImage depthStencilAttachment;
    RPass pass;
};

/// @brief framebuffer handle
struct RFramebuffer : RHandle<struct RFramebufferObj>
{
    uint32_t width() const;
    uint32_t height() const;
};

union RClearColorValue {
    float float32[4];
    int32_t int32[4];
    uint32_t uint32[4];

    // clang-format off
    RClearColorValue(float r, float g, float b, float a)
    {
        float32[0] = r;
        float32[1] = g;
        float32[2] = b;
        float32[3] = a;
    }
    RClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a)
    {
        int32[0] = r;
        int32[1] = g;
        int32[2] = b;
        int32[3] = a;
    }
    RClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
    {
        uint32[0] = r;
        uint32[1] = g;
        uint32[2] = b;
        uint32[3] = a;
    }
    // clang-format on
};

/// @brief render pass instance creation info, used during command list recording
struct RPassBeginInfo
{
    uint32_t clearColorCount;
    RClearColorValue* clearColors;
    RFramebuffer framebuffer;
    RPass pass;
};

/// @brief shader module creation info
struct RShaderInfo
{
    RShaderType type;
    const char* glsl; /// glsl source code string, null terminated
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

/// @brief resource set layout creation info
struct RSetLayoutInfo
{
    uint32_t bindingCount;
    RSetBindingInfo* bindings;
};

/// @brief resource set layout handle, describes all resource bindings within the resource set
struct RSetLayout : RHandle<struct RSetLayoutObj>
{
    uint32_t hash() const;
};

/// @brief describes a type of resource that can be allocated from set pool
struct RSetPoolResource
{
    RBindingType type;
    uint32_t count;
};

/// @brief resource set handle
struct RSet : RHandle<struct RSetObj>
{
};

/// @brief resource set pool creation info
struct RSetPoolInfo
{
    uint32_t maxSets;
    uint32_t resourceCount;
    RSetPoolResource* resources;
};

/// @brief resource set pool handle, used to allocate resource sets
struct RSetPool : RHandle<struct RSetPoolObj>
{
    /// @brief allocate a resource set
    RSet allocate(RSetLayout layout);

    /// @brief returns all allocated set to the pool
    /// @warning all set handles previously allocated will become out of scope
    void reset();
};

/// @brief pipeline layout handle
struct RPipelineLayoutInfo
{
    uint32_t setLayoutCount; /// number of sets the pipeline layout
    RSetLayout* setLayouts;  /// layout of each set, starting at index zero
};

/// @brief pipeline layout handle
struct RPipelineLayout : RHandle<struct RPipelineLayoutObj>
{
    uint32_t hash() const;

    /// @brief get number of resource set layouts
    uint32_t resource_set_count() const;

    /// @brief get the layout of the resource set at index
    RSetLayout resource_set_layout(int32_t index) const;
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
    RPipelineLayout layout;
    RPass pass;
    RPipelineRasterizationInfo rasterization;
    RPipelineBlendInfo blend;
};

/// @brief graphics pipeline handle
struct RPipeline : RHandle<struct RPipelineObj>
{
    /// @brief get the layout of the pipeline
    RPipelineLayout layout() const;
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
    /// @brief free this command list, the handle becomes null afterwards
    void free();

    void begin();
    void end();

    /// @brief begin a render pass instance
    void cmd_begin_pass(const RPassBeginInfo& passBI);

    void cmd_bind_graphics_pipeline(RPipeline pipeline);

    void cmd_bind_graphics_sets(RPipelineLayout layout, uint32_t firstSet, uint32_t setCount, RSet* sets);

    void cmd_bind_vertex_buffers(uint32_t firstBinding, uint32_t bindingCount, RBuffer* buffers);

    void cmd_bind_index_buffer(RBuffer buffer, RIndexType indexType);

    /// @brief draw vertices
    void cmd_draw(const RDrawInfo& drawI);

    /// @brief indexed draw call
    void cmd_draw_indices(const RDrawIndexedInfo& drawI);

    /// @brief end the current render pass instance
    void cmd_end_pass();

    /// @brief add an image memory barrier
    void cmd_image_memory_barrier(RPipelineStageFlags srcStages, RPipelineStageFlags dstStages, const RImageMemoryBarrier& barrier);

    /// @brief a transfer command to copy from buffer to buffer
    void cmd_copy_buffer(RBuffer srcBuffer, RBuffer dstBuffer, uint32_t regionCount, const RBufferCopy* regions);

    /// @brief a transfer command to copy from buffer to image
    void cmd_copy_buffer_to_image(RBuffer srcBuffer, RImage dstImage, RImageLayout dstImageLayout, uint32_t regionCount, const RBufferImageCopy* regions);
};

/// @brief command pool creation info
struct RCommandPoolInfo
{
    bool hintTransient; /// hint to the implementation that command lists allocated from this pool will be short lived
};

/// @brief command pool handle, used to allocate command lists
struct RCommandPool : RHandle<struct RCommandPoolObj>
{
    /// @brief allocate a command list, user must call it's free() method later.
    RCommandList allocate();
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
    RSet set;                /// the resource set to update
    uint32_t dstBinding;
    uint32_t dstArrayIndex;
    uint32_t bufferCount;
    RBindingType bufferBindingType;  /// binding type of the buffer
    RBuffer* buffers;
};

/// @brief render device creation info
struct RDeviceInfo
{
    RDeviceBackend backend;
    GLFWwindow* window;
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

    RPass create_pass(const RPassInfo& passI);
    void destroy_pass(RPass pass);

    RFramebuffer create_framebuffer(const RFramebufferInfo& fbI);
    void destroy_framebuffer(RFramebuffer fbI);

    RCommandPool create_command_pool(const RCommandPoolInfo& poolI);
    void destroy_command_pool(RCommandPool pool);

    RShader create_shader(const RShaderInfo& shaderI);
    void destroy_shader(RShader shader);

    RSetPool create_set_pool(const RSetPoolInfo& poolI);
    void destroy_set_pool(RSetPool pool);

    RSetLayout create_set_layout(const RSetLayoutInfo& layoutI);
    void destroy_set_layout(RSetLayout layout);

    RPipelineLayout create_pipeline_layout(const RPipelineLayoutInfo& layoutI);
    void destroy_pipeline_layout(RPipelineLayout layout);

    RPipeline create_pipeline(const RPipelineInfo& pipelineI);
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

    RImage get_swapchain_color_attachment(uint32_t imageIdx);

    uint32_t get_swapchain_image_count();

    uint32_t get_frames_in_flight_count();

    RQueue get_graphics_queue();
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
