#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/RenderGraph/RGraph.h>

#include "RComponent.h"

namespace LD {

/// @brief Per-frame swapchain state.
struct RGraphSwapchain
{
    RGraphSwapchainInfo info;
    RGraphImageObj* blitSrc = nullptr;
};

enum RGraphState
{
    RGRAPH_STATE_CREATED = 0,
    RGRAPH_STATE_SORTED,
};

/// @brief Per-frame render graph state.
struct RGraphObj
{
    RDevice device;
    RCommandList list;
    RFence frameComplete;
    RGraphPrePassCallback prePassCB = nullptr;
    HashMap<Hash32, RComponent> components;
    Vector<RComponentPassObj*> passOrder;
    HashMap<WindowID, RGraphSwapchain> swapchains;
    uint32_t screenWidth;
    uint32_t screenHeight;
    RGraphState state = RGRAPH_STATE_CREATED;
    void* user = nullptr;

    void sort();
    void record_compute_pass(RComputePassObj* pass, RCommandList list, uint32_t passIdx);
    void record_graphics_pass(RGraphicsPassObj* pass, RCommandList list, uint32_t passIdx);
};

} // namespace LD