#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>
#include <string>
#include <vector>

#define LD_GLSL_VERSION 460
#define LD_GLSL_ENTRY_POINT "main"

namespace LD {

struct RPipelineLayoutObj;

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

struct RShaderPushConstant
{
    uint32_t size;
    uint32_t offset;
    uint32_t uniformArraysize;
    GLSLType uniformGLSLType;
    std::string uniformName;
};

struct RShaderReflection
{
    std::vector<RShaderInput> inputs;
    std::vector<RShaderOutput> outputs;
    std::vector<RShaderBinding> bindings;
    std::vector<RShaderPushConstant> pushConstants;
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

} // namespace LD