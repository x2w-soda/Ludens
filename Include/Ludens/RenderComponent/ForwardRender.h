#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

struct ForwardRenderComponentInfo
{
    RFormat cFormat;
    RFormat dsFormat;
    RClearColorValue clearColor;
    RClearDepthStencilValue clearDS;
    uint32_t width;
    uint32_t height;
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
    inline const char* color_name() const { return "output_color"; }

    /// @brief get the name of the output ID color attachment, with RFORMAT_R32U
    inline const char* id_color_name() const { return "output_id_color"; }

    /// @brief get the name of the output depth stencil attachment
    inline const char* depth_stencil_name() const { return "output_depth_stencil"; }

    /// @brief set the pipeline used to draw RMesh
    /// @param meshPipeline mesh pipeline handle, must adhere to mesh pipeline layout and mesh vertex layout
    void set_mesh_pipeline(RPipeline meshPipeline);

    /// @brief draw a mesh with the most recently bound mesh pipeline
    /// @param mesh mesh handle
    /// @param transform model matrix that transforms mesh to world space
    /// @param id 16 bit identifier written to the id color attachment
    /// @param flags 16 bit flags written to the id color attachment
    void draw_mesh(RMesh mesh, const Mat4& transform, uint16_t id, uint16_t flags);

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
};

} // namespace LD