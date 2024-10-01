#pragma once

namespace LD
{

enum class RBufferType
{
    VertexBuffer = 0,
    IndexBuffer,
    UniformBuffer,
};

enum class RShaderType
{
    VertexShader = 0,
    FragmentShader
};

enum class RResultType
{
    Ok = 0,
    InvalidHandle,
    InvalidIndex,
    ResourceMissing,
    TextureSizeMismatch,
    BufferTypeMismatch,
    ShaderTypeMismatch,
    BindingGroupMismatch,
    BindingMismatch,
    PassBeginError,
    ScissorStackEmpty,
};

enum class RResourceType
{
    Device = 0,
    Texture,
    Buffer,
    Shader,
    FrameBuffer,
    BindingGroupLayout,
    BindingGroup,
    Pipeline,
};

struct RResourceMissing
{
    RResourceType MissingType;
};

struct RTextureSizeMismatch
{
    size_t Expect;
    size_t Actual;
};

struct RBufferTypeMismatch
{
    RBufferType Expect;
    RBufferType Actual;
};

struct RShaderTypeMismatch
{
    RShaderType Expect;
    RShaderType Actual;
};

struct RPassBeginError
{
    size_t NumClearValuesExpect;
    size_t NumClearValuesActual;
    int MissingClearValueIndex;
};

struct RResult
{
    RResultType Type = RResultType::Ok;

    operator bool() const
    {
        return Type == RResultType::Ok;
    }

    union
    {
        RResourceMissing ResourceMissing;
        RTextureSizeMismatch TextureSizeMismatch;
        RBufferTypeMismatch BufferTypeMismatch;
        RShaderTypeMismatch ShaderTypeMismatch;
        RPassBeginError PassBeginError;
    };
};

const char* RResourceTypeString(RResourceType type);

} // namespace LD