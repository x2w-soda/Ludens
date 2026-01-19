#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

enum RGraphImageUsage
{
    RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT = 0,
    RGRAPH_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,
    RGRAPH_IMAGE_USAGE_SAMPLED,
    RGRAPH_IMAGE_USAGE_STORAGE_READ_ONLY,
};

enum NodeType
{
    NODE_TYPE_PRIVATE = 0, /// physical resource, synchronized within its declared component
    NODE_TYPE_OUTPUT,      /// physical resource, can be used as input by another component
    NODE_TYPE_INPUT,       /// reference to some output of another component
    NODE_TYPE_IO,          /// reference to some output of another component, can be used as input by another component
};

struct GraphImage
{
    NodeType type; /// node type in entire render graph
    Hash32 name;   /// declared name in component
    std::string debugName;
    RImageUsageFlags usage;
    RSamplerInfo sampler;
    RFormat format;
    uint32_t width;
    uint32_t height;
};

struct GraphImageRef
{
    NodeType type;
    RComponentObj* srcComponent;
    Hash32 srcOutputName;
};

struct RGraphicsPassColorAttachment
{
    Hash32 name;
    Optional<RClearColorValue> clearValue;
};

struct RGraphicsPassDepthStencilAttachment
{
    Hash32 name;
    Optional<RClearDepthStencilValue> clearValue;
};

struct RComponentPassObj
{
    Hash32 name;                                   /// hash of user declared name
    std::string debugName;                         /// name for debugging
    RComponent component;                          /// owning component
    RPipelineStageFlags stageFlags;                /// compute pass stages
    RAccessFlags accessFlags;                      /// compute pass access
    void* userData;                                /// arbitrary user data
    bool isCallbackScope;                          /// whether the component is within the RCommandList recording scope
    bool isComputePass;                            /// distinguishes between a GraphicsPass and ComputePass
    HashMap<Hash32, RGraphImageUsage> imageUsages; /// track usages of images in this component
    HashSet<RComponentPassObj*> edges;             /// dependency passes
};

struct RGraphicsPassObj : RComponentPassObj
{
    uint32_t width;
    uint32_t height;
    RPassDependency passDep;
    RGraphicsPassCallback callback;                             /// command recording callback for the graphics pass
    Vector<RGraphicsPassColorAttachment> colorAttachments;      /// graphics pass color attachment description
    Vector<RPassColorAttachment> colorAttachmentInfos;          /// consumed by the render backend API
    Vector<RPassResolveAttachment> resolveAttachmentInfos;      /// consumed by the render backend API
    HashSet<Hash32> sampledImages;                              /// all images sampled in this pass
    RGraphicsPassDepthStencilAttachment depthStencilAttachment; /// graphics pass depth stencil attachment description
    RPassDepthStencilAttachment depthStencilAttachmentInfo;     /// consumed by the render backend API
    RSampleCountBit samples;                                    /// if multi-sampled, color attachments are resolved in this pass
    bool hasDepthStencil;

    inline bool operator==(const RGraphicsPassObj& other) const { return name == other.name; }
    inline bool operator!=(const RGraphicsPassObj& other) const { return !operator==(other); }
};

struct RComputePassObj : RComponentPassObj
{
    RComputePassCallback callback; /// user callback for compute operations
    HashSet<Hash32> storageImages; /// all storage images in this pass

    inline bool operator==(const RGraphicsPassObj& other) const { return name == other.name; }
    inline bool operator!=(const RGraphicsPassObj& other) const { return !operator==(other); }
};

struct RComponentObj
{
    Hash32 name;
    RSampleCountBit samples;
    std::string debugName;
    Vector<RComponentPassObj*> passOrder;
    HashMap<Hash32, RComponentPassObj*> passes; /// all passes declared this frame
    HashMap<Hash32, GraphImage> images;         /// name to images declared in this frame
    HashMap<Hash32, GraphImageRef> imageRefs;

    inline bool operator==(const RComponentObj& other) const { return name == other.name; }
    inline bool operator!=(const RComponentObj& other) const { return !operator==(other); }
};

struct RGraphSwapchain
{
    RGraphSwapchainInfo info;
    RComponentObj* blitCompObj = nullptr;
    Hash32 blitOutputName;
};

struct RGraphObj
{
    RDevice device;
    RCommandList list;
    RFence frameComplete;
    HashMap<Hash32, RComponent> components;
    Vector<RComponentPassObj*> passOrder;
    HashMap<WindowID, RGraphSwapchain> swapchains;
    uint32_t screenWidth;
    uint32_t screenHeight;
};

} // namespace LD