#pragma once

#include <string>
#include "Core/Header/Include/Types.h"
#include "Core/OS/Include/UID.h"
#include "Core/DSA/Include/View.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderBase/Include/RBinding.h"

namespace LD
{

class RBindingGroupLayout;

enum class RPrimitiveTopology
{
    TriangleList = 0,
    LineList,
};

enum class RAttributePollRate
{
    PerVertex = 0, // new vertex attributes are polled from vertex buffer slot for every vertex
    PerInstance,   // new vertex attributes are polled from vertex buffer slot for every instance
};

struct RVertexAttribute
{
    u32 Location;   // location of attribute in the vertex shader
    RDataType Type; // attribute data type in the vertex shader
    bool IsNormalized = false;
};

struct RVertexBufferSlot
{
    View<RVertexAttribute> Attributes;
    RAttributePollRate PollRate = RAttributePollRate::PerVertex;
};

// the layout of attributes in vertices during pipeline input assembly
struct RVertexLayout
{
    View<RVertexBufferSlot> Slots;
};

/// Plain old data struct that carries the same information as RPipelineLayout,
/// but does not reference any created resources such as RBindingGroupLayout.
struct RPipelineLayoutData
{
    Vector<RBindingGroupLayoutData> GroupLayouts;
};

struct RPipelineLayout
{
    View<RBindingGroupLayout> GroupLayouts;

    /// @brief reflect the pipeline layout into a plain old data struct
    /// @return true if the pipeline layout is valid
    bool ToData(RPipelineLayoutData& data) const;
};

enum class RCullMode
{
    BackFace,
    FrontFace,
    None,
};

enum class RPolygonMode
{
    Fill,
    Line,
    Point,
};

enum class RBlendMode
{
    Add,
};

enum class RCompareMode
{
    Less = 0,
    LessEqual,
};

enum class RBlendFactor
{
    Zero,
    One,
    SrcAlpha,
    DstAlpha,
    OneMinusSrcAlpha,
    OneMinusDstAlpha,
};

// Info to create a graphics pipeline.
struct RPipelineInfo
{
    const char* Name = nullptr;
    RPrimitiveTopology PrimitiveTopology = RPrimitiveTopology::TriangleList;
    RPipelineLayout PipelineLayout;
    RVertexLayout VertexLayout;
    RShader VertexShader;
    RShader FragmentShader;
    RPass RenderPass;

    struct
    {
        bool BlendEnabled = false;
        RBlendFactor ColorSrcFactor = RBlendFactor::SrcAlpha;
        RBlendFactor ColorDstFactor = RBlendFactor::OneMinusSrcAlpha;
        RBlendMode ColorBlendMode = RBlendMode::Add;
        RBlendFactor AlphaSrcFactor = RBlendFactor::One;
        RBlendFactor AlphaDstFactor = RBlendFactor::Zero;
        RBlendMode AlphaBlendMode = RBlendMode::Add;
    } BlendState;

    struct
    {
        bool DepthTestEnabled = true;
        bool DepthWriteEnabled = true;
        RCompareMode DepthCompareMode = RCompareMode::Less;
    } DepthStencilState;

    struct
    {
        RCullMode CullMode = RCullMode::BackFace;
        RPolygonMode PolygonMode = RPolygonMode::Fill;
    } RasterizationState;
};

struct RPipelineBase;
struct RPipelineGL;
struct RPipelineVK;

/// graphics pipeline handle and interface
class RPipeline : public RHandle<RPipelineBase>
{
};

} // namespace LD
