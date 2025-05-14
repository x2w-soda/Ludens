#pragma once

#include <Ludens/Header/Name.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace LD {

struct RGraphicsPassHash
{
    std::size_t operator()(const RGraphicsPass& pass) const
    {
        return (std::size_t)pass.rid();
    }
};

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

struct RGraphicsPassObj
{
    Name name;
    std::string debugName;
    uint32_t width;
    uint32_t height;
    RComponent component; /// owning component
    RPassDependency passDep;
    RGraphicsPassCallback callback;
    std::unordered_map<uint32_t, RGraphImageUsage> imageUsages;
    std::unordered_set<RGraphicsPass, RGraphicsPassHash> edges; /// dependency passes
    std::vector<RGraphicsPassColorAttachment> colorAttachments; /// graphics pass color attachment description
    std::vector<RPassColorAttachment> colorAttachmentInfos;     /// consumed by the render backend API
    std::unordered_set<Name, NameHash> sampledImages;           /// all images sampled in this pass
    RGraphicsPassDepthStencilAttachment depthStencilAttachment; /// graphics pass depth stencil attachment description
    RPassDepthStencilAttachment depthStencilAttachmentInfo;     /// consumed by the render backend API
    RPipelineStageFlags stageFlags;                             /// render pass stages
    RAccessFlags accessFlags;                                   /// render pass access
    void* userData;
    bool isCallbackScope;
    bool hasDepthStencil;

    inline bool operator==(const RGraphicsPassObj& other) const { return name == other.name; }
    inline bool operator!=(const RGraphicsPassObj& other) const { return !operator==(other); }
};

struct RComponentObj
{
    Name name;
    std::string debugName;
    std::vector<RGraphicsPass> graphicsPassOrder;
    std::unordered_map<Name, RGraphicsPass, NameHash> graphicsPasses; /// name graphics passes declared this frame
    std::unordered_map<Name, GraphImage, NameHash> images;            /// name to images declared in this frame
    std::unordered_map<Name, GraphImageRef, NameHash> imageRefs;

    inline bool operator==(const RComponentObj& other) const { return name == other.name; }
    inline bool operator!=(const RComponentObj& other) const { return !operator==(other); }
};

struct RGraphObj
{
    RGraphInfo info;
    RCommandList list;
    std::unordered_map<Name, RComponent, NameHash> components;
    std::vector<RGraphicsPass> passOrder;
    RComponentObj* blitCompObj;
    Name blitOutputName;
};

} // namespace LD