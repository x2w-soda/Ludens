#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/DSA/Include/Optional.h"
#include "Core/Math/Include/Vec4.h"
#include "Core/OS/Include/UID.h"

namespace LD
{

enum class RBackend
{
    OpenGL = 0,
    Vulkan,
};

enum class RMemoryUsage
{
    // memory that is initialized and uploaded once to GPU during resource creation.
    // note that the memory is only immutable from the CPU side, GPU can still write.
    Immutable = 0,

    // memory that is expected to be written by CPU every frame.
    FrameDynamic,
};

struct RDrawVertexInfo
{
    u32 VertexCount = 0;
    u32 VertexStart = 0;
    u32 InstanceCount = 1;
    u32 InstanceStart = 0;
};

enum class RIndexType
{
    u16 = 0,
    u32
};

struct RDrawIndexedInfo
{
    u32 IndexCount = 0;
    u32 IndexStart = 0;
    u32 InstanceCount = 1;
    u32 InstanceStart = 0;
};

using RClearColorValue = Vec4;

struct RClearDepthStencilValue
{
    f32 Depth = 1.0f;
    u8 Stencil = 0;
};

struct RClearValue
{
    Optional<RClearColorValue> Color;
    Optional<RClearDepthStencilValue> DepthStencil;
};

/// Renderer object handle, not ref-counted. Handles are extremely cheap to copy around,
/// but should be treated as raw pointers with caution.
template <typename TBase>
class RHandle
{
    friend TBase;

public:
    using Base = TBase;

    operator TBase*() const
    {
        return mBase;
    }

    operator bool() const
    {
        return mID != 0 && mBase != nullptr;
    }

    explicit operator UID() const
    {
        return mID;
    }

    inline bool operator==(const RHandle& other) const
    {
        if (other.mID == 0 || mID == 0)
            return false;

        return other.mID == mID;
    }

    inline bool operator!=(const RHandle& other) const
    {
        return !(*this == other);
    }

    inline void ResetHandle()
    {
        mID = 0;
        mBase = nullptr;
    }

protected:
    inline void SetHandle(UID id, TBase* base)
    {
        mID = id;
        mBase = base;
        LD_DEBUG_ASSERT((bool)*this);
    }

    UID mID = 0;
    TBase* mBase = nullptr;
};

} // namespace LD