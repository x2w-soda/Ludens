#pragma once

#include "Core/RenderBase/Include/RTypes.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RResult.h"

namespace LD
{

struct RBufferInfo
{
    RBufferType Type;
    RMemoryUsage MemoryUsage;
    const void* Data = nullptr;
    u32 Size = 0;
};

struct RBufferBase;
struct RBufferGL;

// render buffer handle and interface
class RBuffer : public RHandle<RBufferBase>
{
    friend struct RBufferGL;

public:
    RResult SetData(u32 offset, u32 size, const void* data);
};

} // namespace LD
