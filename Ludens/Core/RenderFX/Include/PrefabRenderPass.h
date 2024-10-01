#pragma once

#include "Core/RenderBase/Include/RPass.h"

namespace LD
{

/// base class for first-party render passes
class PrefabRenderPass
{
public:
    PrefabRenderPass() = default;
    PrefabRenderPass(const PrefabRenderPass&) = delete;
    ~PrefabRenderPass()
    {
        LD_DEBUG_ASSERT(!mHandle);
    }

    PrefabRenderPass& operator=(const PrefabRenderPass&) = delete;

    operator bool() const
    {
        return (bool)mHandle;
    }

    explicit operator RPass() const
    {
        LD_DEBUG_ASSERT(mHandle);
        return mHandle;
    }

protected:
    RPass mHandle;
};

} // namespace LD