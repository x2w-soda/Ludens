#pragma once

#include "Core/RenderBase/Include/RFrameBuffer.h"

namespace LD
{

/// base class for first-party frame buffers
class PrefabFrameBuffer
{
public:
    PrefabFrameBuffer() = default;
    PrefabFrameBuffer(const PrefabFrameBuffer&) = delete;
    ~PrefabFrameBuffer()
    {
        LD_DEBUG_ASSERT(!mHandle);
    }

    PrefabFrameBuffer& operator=(const PrefabFrameBuffer&) = delete;

    operator bool() const
    {
        return (bool)mHandle;
    }

    explicit operator RFrameBuffer() const
    {
        LD_DEBUG_ASSERT(mHandle);
        return mHandle;
    }

protected:
    RFrameBuffer mHandle;
};

} // namespace LD