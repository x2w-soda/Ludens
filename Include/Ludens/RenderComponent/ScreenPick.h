#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <cstdint>
#include <vector>

namespace LD {

struct ScreenPickComponentInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t pickQueryCount;
    Vec2* pickPositions;
};

struct ScreenPickResult
{
    Vec2 pos;
    uint16_t id;
    uint16_t flags;
};

struct ScreenPickComponent : Handle<struct ScreenPickComponentObj>
{
    static ScreenPickComponent add(RGraph graph, const ScreenPickComponentInfo& componentI);

    void get_results(std::vector<ScreenPickResult>& results);

    /// @brief get the name of the component
    inline const char* component_name() const { return "screen_pick"; }

    /// @brief get the name of the input ID color attachment
    inline const char* input_name() const { return "input"; }
};

} // namespace LD