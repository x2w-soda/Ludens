#pragma once

#include "Core/RenderBase/Include/RDevice.h"

namespace LD
{

class RenderResources
{
public:
    RenderResources() = default;
    RenderResources(const RenderResources&) = delete;
    RenderResources(RenderResources&&) = delete;
    ~RenderResources()
    {
        LD_DEBUG_ASSERT(!mDevice);
    }

    RenderResources& operator=(const RenderResources&) = delete;
    RenderResources& operator=(RenderResources&&) = delete;

    operator bool() const
    {
        return (bool)mDevice;
    }

protected:
    RDevice mDevice{};
};

} // namespace LD