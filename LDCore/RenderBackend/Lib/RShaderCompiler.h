#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>
#include <string>
#include <vector>

#define LD_GLSL_VERSION 460
#define LD_GLSL_ENTRY_POINT "main"

namespace LD {

struct RShaderBinding
{
    uint32_t setIndex;
    uint32_t bindingIndex;
    uint32_t arrayCount;
    std::string name;
    RBindingType type;
};

struct RShaderReflection
{
    std::vector<RShaderBinding> bindings;
};

class RShaderCompiler
{
public:
    /// @brief Compiles Vulkan-GLSL of shader type to SPIR-V
    bool compile_to_spirv(RShaderType type, const char* vkGLSL, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection);

    /// @brief Decompiles SPIRV back to OpenGL-compatible GLSL
    bool decompile_to_opengl_glsl(const std::vector<uint32_t>& spirvCode, std::string& glGLSL);
};

} // namespace LD