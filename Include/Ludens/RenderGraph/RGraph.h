#pragma once

#include <Ludens/Header/Name.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct RGraphicsPassInfo
{
    const char* name;
    uint32_t width;
    uint32_t height;
};

struct RGraphicsPass : RHandle<struct RGraphicsPassObj>
{
    /// @brief get declared graphics pass name
    Name name() const;

    /// @brief declare to use an image as sampled
    void use_image_sampled(Name name);

    /// @brief declare to use an image as color attachment
    /// @param name name of the color attachment declared in component
    /// @param loadOp what to do with the color attachment when the pass begins
    /// @param clear the clear value used if loadOp is RATTACHMENT_LOAD_OP_CLEAR
    void use_color_attachment(Name name, RAttachmentLoadOp loadOp, const RClearColorValue* clear);

    /// @brief declare to use an image as depth stencil attachment
    /// @param name name of the depth stencil attachment declared in component
    /// @param loadOp what to do with the depth stencil attachment when the pass begins
    /// @param clear the clear value used if loadOp is RATTACHMENT_LOAD_OP_CLEAR
    void use_depth_stencil_attachment(Name name, RAttachmentLoadOp loadOp, const RClearDepthStencilValue* clear);

    /// @brief get the actual image declared by use_image
    RImage get_image(Name name, RImageLayout* layout = nullptr);
};

typedef void (*RGraphicsPassCallback)(RGraphicsPass pass, RCommandList list, void* userData);

struct RComputePassInfo
{
    const char* name;
};

struct RComputePass : RHandle<struct RComputePassObj>
{
    /// @brief get declared compute pass name
    Name name() const;

    /// @brief declare to use a storage image as read only
    /// @param name name of the image declared in component
    void use_image_storage_read_only(Name name);

    /// @brief get the actual image declared by use_image
    RImage get_image(Name name);
};

typedef void (*RComputePassCallback)(RComputePass pass, RCommandList list, void* userData);

/// @brief render component handle
///        input resources are output resources of another component
struct RComponent : RHandle<struct RComponentObj>
{
    /// @brief get declared component name
    Name name() const;

    /// @brief declare an image that can only be used within the component
    void add_private_image(const char* name, RFormat format, uint32_t width, uint32_t height, RSamplerInfo* sampler = nullptr);

    /// @brief declare an image that can be used by another component as input
    void add_output_image(const char* name, RFormat format, uint32_t width, uint32_t height, RSamplerInfo* sampler = nullptr);

    /// @brief declare an image that references some output image of another component
    void add_input_image(const char* name, RFormat format, uint32_t width, uint32_t height);

    /// @brief declare an image that references some output image of another component,
    ///        and can be used by another component as input.
    void add_io_image(const char* name, RFormat format, uint32_t width, uint32_t height);

    /// @brief declare a graphics pass in this component for this frame
    RGraphicsPass add_graphics_pass(const RGraphicsPassInfo& gpI, void* userData, RGraphicsPassCallback callback);

    /// @brief declare a compute pass in this component for this frame
    RComputePass add_compute_pass(const RComputePassInfo& cpI, void* userData, RComputePassCallback callback);
};

/// @brief render graph creation info
struct RGraphInfo
{
    RDevice device;
    RCommandList list;
    RImage swapchainImage;
    RFence frameComplete;
    RSemaphore imageAcquired;
    RSemaphore presentReady;
};

/// @brief render graph handle
struct RGraph : RHandle<struct RGraphObj>
{
    typedef void (*OnReleaseCallback)(void* user);
    typedef void (*OnDestroyCallback)(void* user);

    static RGraph create(const RGraphInfo& graphI);
    static void destroy(RGraph graph);

    /// @brief destroy all resources used by render graph across frames
    static void release(RDevice device);

    /// @brief component implementations may add a callback that will be called after the graph submission.
    static void add_destroy_callback(void* user, OnDestroyCallback onDestroy);

    /// @brief component implementations may add a callback that will be called at RGraph::release to release resources.
    static void add_release_callback(void* user, OnReleaseCallback onRelease);

    /// @brief get the render device this graph is created with
    RDevice get_device();

    /// @brief get the swapchain image handle for this frame
    RImage get_swapchain_image();

    /// @brief declare a component for this frame
    RComponent add_component(const char* name);

    /// @brief connects an output image of a Component to some input image of another Component
    void connect_image(const char* srcComp, const char* srcOutImage, const char* dstComp, const char* dstInImage);

    /// @brief connects an output image of a Component to the swapchain image of this frame.
    ///        In practice, this is equivalent to a framebuffer blit.
    void connect_swapchain_image(const char* srcComp, const char* srcOutImage);

    void submit(bool save = false);
};

} // namespace LD