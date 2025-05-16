#pragma once

#include <Ludens/Header/Name.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace LD {

struct NameHash
{
    std::size_t operator()(const Name& name) const
    {
        return (std::size_t)name;
    }
};

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
    Name name;     /// declared name in component
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
    Name srcOutputName;
};

struct RGraphicsPassColorAttachment
{
    Name name;
    std::optional<RClearColorValue> clearValue;
};

struct RGraphicsPassDepthStencilAttachment
{
    Name name;
    std::optional<RClearDepthStencilValue> clearValue;
};

struct RComponentPassObj
{
    Name name;
    std::string debugName;
    RComponent component;           /// owning component
    RPipelineStageFlags stageFlags; /// compute pass stages
    RAccessFlags accessFlags;       /// compute pass access
    void* userData;
    bool isCallbackScope;
    bool isComputePass;
    std::unordered_map<uint32_t, RGraphImageUsage> imageUsages;
    std::unordered_set<RComponentPassObj*> edges; /// dependency passes
};

struct RGraphicsPassObj : RComponentPassObj
{
    uint32_t width;
    uint32_t height;
    RPassDependency passDep;
    RGraphicsPassCallback callback;
    std::vector<RGraphicsPassColorAttachment> colorAttachments; /// graphics pass color attachment description
    std::vector<RPassColorAttachment> colorAttachmentInfos;     /// consumed by the render backend API
    std::unordered_set<Name, NameHash> sampledImages;           /// all images sampled in this pass
    RGraphicsPassDepthStencilAttachment depthStencilAttachment; /// graphics pass depth stencil attachment description
    RPassDepthStencilAttachment depthStencilAttachmentInfo;     /// consumed by the render backend API
    bool hasDepthStencil;

    inline bool operator==(const RGraphicsPassObj& other) const { return name == other.name; }
    inline bool operator!=(const RGraphicsPassObj& other) const { return !operator==(other); }
};

struct RComputePassObj : RComponentPassObj
{
    RComputePassCallback callback;                    /// user callback for compute operations
    std::unordered_set<Name, NameHash> storageImages; /// all storage images in this pass

    inline bool operator==(const RGraphicsPassObj& other) const { return name == other.name; }
    inline bool operator!=(const RGraphicsPassObj& other) const { return !operator==(other); }
};

struct RComponentObj
{
    Name name;
    std::string debugName;
    std::vector<RComponentPassObj*> passOrder;
    std::unordered_map<Name, RComponentPassObj*, NameHash> passes; /// all passes declared this frame
    std::unordered_map<Name, GraphImage, NameHash> images;         /// name to images declared in this frame
    std::unordered_map<Name, GraphImageRef, NameHash> imageRefs;

    inline bool operator==(const RComponentObj& other) const { return name == other.name; }
    inline bool operator!=(const RComponentObj& other) const { return !operator==(other); }
};

struct RGraphObj
{
    RGraphInfo info;
    RCommandList list;
    std::unordered_map<Name, RComponent, NameHash> components;
    std::vector<RComponentPassObj*> passOrder;
    RComponentObj* blitCompObj;
    Name blitOutputName;
};

} // namespace LD