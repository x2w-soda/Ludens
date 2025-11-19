#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>
#include <string>
#include <vector>

#define LD_GLSL_VERSION 460
#define LD_GLSL_ENTRY_POINT "main"

namespace LD {

struct RPipelineLayoutObj;

enum GLSLType
{
    GLSL_TYPE_STRUCT = 0,
    GLSL_TYPE_FLOAT,
    GLSL_TYPE_VEC2,
    GLSL_TYPE_VEC3,
    GLSL_TYPE_VEC4,
    GLSL_TYPE_DOUBLE,
    GLSL_TYPE_DVEC2,
    GLSL_TYPE_DVEC3,
    GLSL_TYPE_DVEC4,
    GLSL_TYPE_UINT,
    GLSL_TYPE_UVEC2,
    GLSL_TYPE_UVEC3,
    GLSL_TYPE_UVEC4,
    GLSL_TYPE_INT,
    GLSL_TYPE_IVEC2,
    GLSL_TYPE_IVEC3,
    GLSL_TYPE_IVEC4,
    GLSL_TYPE_BOOL,
    GLSL_TYPE_BVEC2,
    GLSL_TYPE_BVEC3,
    GLSL_TYPE_BVEC4,
    GLSL_TYPE_MAT4,
    GLSL_TYPE_SAMPLER_2D,
    GLSL_TYPE_SAMPLER_CUBE,
    GLSL_TYPE_USAMPLER_2D,
    GLSL_TYPE_UIMAGE_2D,
    GLSL_TYPE_ENUM_COUNT,
};

struct RShaderBinding
{
    std::string name;
    uint32_t setIndex;
    uint32_t bindingIndex;
    uint32_t arrayCount;
    RBindingType type;
    GLSLType glslType;
};

struct RShaderLocation
{
    std::string name;
    uint32_t location;
    uint32_t arrayCount;
    GLSLType glslType;
};

using RShaderInput = RShaderLocation;
using RShaderOutput = RShaderLocation;

struct RShaderReflection
{
    std::vector<RShaderInput> inputs;
    std::vector<RShaderOutput> outputs;
    std::vector<RShaderBinding> bindings;
};

struct RShaderOpenGLBindingRemap
{
    uint32_t vkSetIndex;
    uint32_t vkBindingIndex;
    uint32_t glBindingIndex;
};

struct RShaderOpenGLRemap
{
    std::vector<RShaderOpenGLBindingRemap> bindingRemaps;

    const RShaderOpenGLBindingRemap* get_binding_remap(uint32_t vkSetIndex, uint32_t vkBindingIndex) const;
};

class RShaderCompiler
{
public:
    /// @brief Compiles Vulkan-GLSL of shader type to SPIR-V
    bool compile_to_spirv(RShaderType type, const char* vkGLSL, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection);

    /// @brief Compute remap table that maps Vulkan GLSL layout qualifiers to OpenGL qualifiers.
    bool compute_opengl_remap(const RPipelineLayoutObj* layoutObj, RShaderOpenGLRemap& remap);

    /// @brief Decompiles SPIRV back to OpenGL-compatible GLSL
    bool decompile_to_opengl_glsl(const RShaderOpenGLRemap& remap, const std::vector<uint32_t>& spirvCode, std::string& glGLSL);
};

/// @brief Get static c string of GLSL data type.
const char* get_glsl_type_cstr(GLSLType type);

} // namespace LD