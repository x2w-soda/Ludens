#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

struct ForwardRenderComponentInfo
{
    uint32_t width;
    uint32_t height;
    RFormat colorFormat;
    RFormat depthStencilFormat;
    RSampleCountBit samples;                   /// number of samples for MSAA
    RClearColorValue clearColor;               /// color clear value
    RClearDepthStencilValue clearDepthStencil; /// depth stencil clear value
};

/// @brief Forward Rendering, this is one of the root render components
///        where no input is required.
struct ForwardRenderComponent : Handle<struct ForwardRenderComponentObj>
{
    typedef void (*RenderCallback)(ForwardRenderComponent renderer, void* user);

    /// @brief adds the component to render graph
    static ForwardRenderComponent add(RGraph graph, const ForwardRenderComponentInfo& componentI, RSet frameSet, RenderCallback callback, void* user);

    /// @brief get the name of this component
    inline const char* component_name() const { return "forward"; }

    /// @brief get the name of the output color attachment
    inline const char* out_color_name() const { return "out_color"; }

    /// @brief get the name of the output ID-flags color attachment, with RFORMAT_RGBA8U
    inline const char* out_idflags_name() const { return "out_idflags"; }

    /// @brief get the name of the output depth stencil attachment
    inline const char* out_depth_stencil_name() const { return "out_depth_stencil"; }

    /// @brief set the pipeline used to draw RMesh
    /// @param meshPipeline mesh pipeline handle, must adhere to mesh pipeline layout and mesh vertex layout
    void set_mesh_pipeline(RPipeline meshPipeline);

    /// @brief dependency injection to configure the push constant for the current mesh pipeline
    /// @param layout corresponding layout of the pipeline bound with set_mesh_pipeline
    /// @param offset push constant offset
    /// @param size push constant size in bytes
    /// @param pc puch constant data
    void set_push_constant(RPipelineLayoutInfo layout, uint32_t offset, uint32_t size, const void* pc);

    /// @brief draw a mesh with the most recently bound mesh pipeline
    /// @param mesh mesh handle
    void draw_mesh(RMesh mesh);

    /// @brief draw a line from p0 to p1
    /// @param p0 starting world position
    /// @param p1 ending world position
    /// @param color line color
    void draw_line(const Vec3& p0, const Vec3& p1, uint32_t color);

    /// @brief draw 12 lines of an AABB
    /// @param min minimum position of box
    /// @param max maximum position of box
    /// @param color outline color
    void draw_aabb_outline(const Vec3& min, const Vec3& max, uint32_t color);

    /// @brief draw the skybox using the environment cubemap bound in the FrameSet
    void draw_skybox();
};

} // namespace LD