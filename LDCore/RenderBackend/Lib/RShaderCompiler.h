#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <vector>
#include <string>

#define LD_GLSL_VERSION 460
#define LD_GLSL_ENTRY_POINT "main"

namespace LD {

class RShaderCompiler
{
public:

    /// @brief Compiles Vulkan-GLSL of shader type to SPIR-V
    bool compile_to_spirv(RShaderType type, const char* vkGLSL, std::vector<uint32_t>& spirvCode);

    /// @brief Compiles Vulkan-GLSL of shader type to OpenGL GLSL
    bool compile_to_opengl_glsl(RShaderType type, const char* vkGLSL, std::string& glGLSL);
};

} // namespace LD