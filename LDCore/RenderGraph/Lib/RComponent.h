#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

struct RComponentObj;
struct RComponentPassObj;
struct RGraphImageObj;

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

struct RGraphImageObj
{
    NodeType type;          /// node type in entire render graph
    Hash32 name;            /// declared name in component
    RComponentObj* compObj; /// owning component
    std::string debugName;
    RImageUsageFlags usage;
    RSamplerInfo sampler{};
    RFormat format;
    uint32_t width;
    uint32_t height;

    inline bool is_input_image() const { return type == NODE_TYPE_INPUT || type == NODE_TYPE_IO; }
    inline bool is_output_image() const { return type == NODE_TYPE_OUTPUT || type == NODE_TYPE_IO; }
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
    std::string debugName;                         /// name for debugging, globally unique
    RComponentObj* compObj;                        /// owning component
    RPipelineStageFlags stageFlags;                /// compute pass stages
    RAccessFlags accessFlags;                      /// compute pass access
    void* user;                                    /// arbitrary user data
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

/// @brief Per-frame render component state.
struct RComponentObj
{
    Hash32 name;
    RSampleCountBit samples;
    std::string debugName; /// globally unique
    Vector<RComponentPassObj*> passOrder;
    HashMap<Hash32, RComponentPassObj*> passes; /// all passes declared in this component
    HashMap<Hash32, RGraphImageObj*> images;    /// all images declared in this component
    HashMap<Hash32, RGraphImageObj*> imageRefs; /// for input and IO images, reference the an image from some upstream component

    inline bool operator==(const RComponentObj& other) const { return name == other.name; }
    inline bool operator!=(const RComponentObj& other) const { return !operator==(other); }

    RGraphImageObj* create_image(NodeType type, const char* nameStr, RFormat format, uint32_t width, uint32_t height, const RSamplerInfo* sampler);
};

} // namespace LD