#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

/// @brief Renders a Skybox on top of a color attachment
struct SkyboxComponent : Handle<struct SkyboxComponentObj>
{
    /// @brief adds the component to render graph
    static SkyboxComponent add(RGraph graph, RFormat cFormat, RFormat dsFormat, uint32_t width, uint32_t height);

    /// @brief get the name of this component
    inline const char* component_name() const { return "skybox"; }

    /// @brief get the name of the IO color attachment
    inline const char* io_color_name() const { return "io_color"; }

    /// @brief get the name of the IO depth stencil attachment
    inline const char* io_depth_stencil_name() const { return "io_depth_stencil"; }
};

} // namespace LD