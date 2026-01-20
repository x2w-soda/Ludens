#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

struct DualKawaseComponentInfo
{
    RFormat format;  /// color format
    Color mixColor;  /// mix color RGB, keep alpha channel at 0xFF
    float mixFactor; /// lerp factor between blur color and mix color, 0 performs no blur
};

struct DualKawaseComponent : Handle<struct DualKawaseComponentObj>
{
    /// @brief adds the component to render graph
    static DualKawaseComponent add(RGraph graph, const DualKawaseComponentInfo& info);

    RGraphImage in_color_attachment();
    RGraphImage out_color_attachment();
};

} // namespace LD