#pragma once

#include <string>
#include <glslang/Public/ShaderLang.h>
#include "Core/IO/Include/FileSystem.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/View.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RBinding.h"

namespace LD
{

/// the result of a shader compilation
struct RShaderCompileResult
{
    /// whether the compilation was successful and SPIRV code is generated
    bool Success;

    /// the type of the shader
    RShaderType Type;

    /// a copy of the source GLSL at the time of compilation
    std::string Source;

    /// on failure, the error message and diagnostics
    std::string Error;

    /// on success, the compiled SPIRV data
    Vector<u8> SPIRV;
};

// compiles Ludens cananical GLSL to SPIRV that is compatible with target backend,
// - OpenGL backend eventually passes to glShaderBinary
// - Vulkan backend eventually passes to VkShaderModuleCreateInfo
// the compiler also checks if the GLSL bindings are compatible with the
// pipeline layout, detecting errors early.
class RShaderCompiler
{
public:
    RShaderCompiler() = delete;
    RShaderCompiler(const RShaderCompiler&) = delete;
    ~RShaderCompiler();

    explicit RShaderCompiler(RBackend targetBackend);

    RShaderCompiler& operator=(const RShaderCompiler&) = delete;

    /// @brief Compile Ludens GLSL
    /// @param glsl input Ludens GLSL that contains #ludens macros, describes a single pipeline layout
    ///        and may contain multiple shader stages.
    /// @param results for each shader stage in the input, its corresponding compilation result.
    void Compile(const std::string& glsl, Vector<RShaderCompileResult>& results);

    /// Compile SPIRV for a specific shader stage
    void CompileStage(const RPipelineLayoutData& layout, RShaderType type, const std::string& glsl,
                      RShaderCompileResult& result);

private:
    void GlslangShaderType(RShaderType type, EShLanguage* language);
    void GlslangBackend(RBackend backend, glslang::EShClient* client, glslang::EShTargetClientVersion* version);
    bool GlslangCompile(RBackend backend, RShaderType type, const std::string& glsl, const std::string& glslPreamble,
                        Vector<u32>& spirv, std::string& error);
    void PatchOpenGL(const RPipelineLayoutData& layout, RShaderType type, std::string& glsl);

    int ParseLudensMacro(const std::string& line, RPipelineLayoutData& layout, RShaderType& type);
    int ParseLudensMacroGroupPrefab(std::string str, RBindingGroupLayoutData& layoutData);

    RBackend mTargetBackend;
};

struct RShaderCacheInfo
{
    RDevice Device;
    Path CacheDirectory;
};

// utility service to cache and retrieve spirv binaries on disk,
// this class should have a shorter lifetime than RShaderCacheInfo::Device,
// which is used to create and delete shaders.
class RShaderCache
{
public:
    RShaderCache() = default;
    RShaderCache(const RShaderCache&) = delete;
    ~RShaderCache();

    RShaderCache& operator=(const RShaderCache&) = delete;

    void Startup(const RShaderCacheInfo& info);
    void Cleanup();

    RResult GetOrCreateShader(const RPipelineLayout& layout, RShaderType type, const std::string& glsl,
                              std::string name, RShader& shader);

private:
    struct CacheInfo
    {
        RShaderType ShaderType;
        RShader Shader;
        size_t SourceHash;
    };

    std::unordered_map<std::string, CacheInfo> mCacheInfo;
    RDevice mDevice;
    Path mCacheDirectory;
    bool mHasStartup = false;
};

} // namespace LD