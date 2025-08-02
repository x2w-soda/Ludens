#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

struct DualKawaseComponent : Handle<struct DualKawaseComponentObj>
{
    /// @brief adds the component to render graph
    static DualKawaseComponent add(RGraph graph, RFormat format);

    inline const char* component_name() const { return "dual_kawase"; }
    inline const char* input_name() const { return "input"; }
    inline const char* output_name() const { return "output"; }
};

} // namespace LD