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

/// @brief Per-frame render graph state.
struct RGraphObj
{
    RDevice device;
    RCommandList list;
    RFence frameComplete;
    HashMap<Hash32, RComponent> components;
    Vector<RComponentPassObj*> passOrder;
    HashMap<WindowID, RGraphSwapchain> swapchains;
    uint32_t screenWidth;
    uint32_t screenHeight;

    RImage get_or_create_image(RComponentObj* comp, Hash32 name, RFormat format, uint32_t width, uint32_t height);
    RImage get_or_create_ms_image(RComponentObj* comp, Hash32 name, RFormat format, uint32_t width, uint32_t height);
};

} // namespace LD