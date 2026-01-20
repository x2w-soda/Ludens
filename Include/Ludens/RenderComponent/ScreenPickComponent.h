#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <cstdint>
#include <vector>

namespace LD {

struct ScreenPickComponentInfo
{
    uint32_t pickQueryCount;
    const Vec2* pickPositions;
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

    RGraphImage attachment();

    void get_results(std::vector<ScreenPickResult>& results);
};

} // namespace LD