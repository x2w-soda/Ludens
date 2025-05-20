#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

struct OutlineComponentInfo
{
    RFormat format;
    uint32_t width;
    uint32_t height;
};

/// @brief A component to draw outline using a post process pass.
struct OutlineComponent : Handle<struct OutlineComponentObj>
{
    /// @brief add a outline post process pass to render graph
    /// @param graph the render graph
    /// @param info post process configuration
    /// @return the component handle
    static OutlineComponent add(RGraph& graph, const OutlineComponentInfo& info);

    /// @brief get the name of the component
    /// @warning returned pointer is transient
    const char* component_name() const;

    /// @brief get the name of the input ID color attachment
    inline const char* input_name() const { return "input"; }

    /// @brief get the name of the IO color attachment
    inline const char* io_name() const { return "io"; }
};

} // namespace LD