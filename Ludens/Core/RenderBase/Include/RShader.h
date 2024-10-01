#pragma once

#include <string>
#include <unordered_map>
#include "Core/OS/Include/UID.h"
#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/RResult.h"
#include "Core/RenderBase/Include/RDevice.h"

namespace LD
{

struct RPipelineLayout;
struct RPipelineLayoutData;

enum class RShaderSourceType
{
    GLSL,
    SPIRV,
};

enum class RDataType
{
    Float,
    Vec2,
    Vec3,
    Vec4,
};

struct RShaderInfo
{
    const char* Name = nullptr;
    RShaderType Type;
    RShaderSourceType SourceType;
    const void* Data = nullptr;
    u32 Size = 0;
};

struct RShaderBase;
struct RShaderGL;

/// shader handle and interface
class RShader : public RHandle<RShaderBase>
{
public:
};

} // namespace LD
