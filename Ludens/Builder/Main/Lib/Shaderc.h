#pragma once

#include <string>
#include "Core/DSA/Include/Vector.h"
#include "Core/IO/Include/FileSystem.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderFX/Include/RShaderCompiler.h"

namespace LD
{


/// the builder's shader compiler mode,
/// compiles Ludens canonical GLSL to SPIRV for OpenGL and Vulkan backend
class Shaderc
{
public:
    Shaderc();
    Shaderc(const Shaderc&) = delete;
    ~Shaderc();

    Shaderc& operator=(const Shaderc&) = delete;

    int Main(int argc, const char** argv);

    void Compile(RBackend target, const Path& inputPath, const char* data, size_t size);

    void Compile(RBackend target, const std::string& glsl, Vector<RShaderCompileResult>& results);

private:
    bool mOpenGL;
    bool mVulkan;
    std::string mOutputDir;
};

} // namespace LD