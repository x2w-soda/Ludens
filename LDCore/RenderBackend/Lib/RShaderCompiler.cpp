#include "RShaderCompiler.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <iostream>
// hide from module user
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#define LD_GLSLANG_VULKAN_CLIENT_VERSION glslang::EShTargetVulkan_1_3
#define LD_GLSLANG_TARGET_SPIRV_VERSION glslang::EShTargetSpv_1_0

namespace LD {

static EShLanguage get_glslang_shader_stage(RShaderType type)
{
    EShLanguage stage{};

    switch (type)
    {
    case RSHADER_TYPE_COMPUTE:
        stage = EShLangCompute;
        break;
    case RSHADER_TYPE_VERTEX:
        stage = EShLangVertex;
        break;
    case RSHADER_TYPE_FRAGMENT:
        stage = EShLangFragment;
        break;
    default:
        LD_UNREACHABLE;
    }

    return stage;
}

static bool glslang_compile_glsl(glslang::EShClient client, glslang::EshTargetClientVersion clientVersion, EShLanguage stage, const char* glsl, std::vector<uint32_t>& spirvCode)
{
    static bool hasInitialized = false;

    if (!hasInitialized)
    {
        LD_PROFILE_SCOPE_NAME("glslang::InitializeProcess");

        glslang::InitializeProcess();
        hasInitialized = true;
    }

    glslang::TShader shader(stage);
    shader.setStrings(&glsl, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, LD_GLSL_VERSION);
    shader.setEnvClient(client, clientVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, LD_GLSLANG_TARGET_SPIRV_VERSION);
    shader.setEntryPoint(LD_GLSL_ENTRY_POINT);
    shader.setSourceEntryPoint(LD_GLSL_ENTRY_POINT);
    shader.setDebugInfo(true);

    const EShMessages messages = EShMsgDefault;
    const TBuiltInResource* resources = ::GetDefaultResources();
    glslang::TShader::ForbidIncluder includer{};

    if (!shader.parse(resources, LD_GLSL_VERSION, false, messages, includer))
    {
        std::cout << "glslang_compile_glsl compilation failed: " << std::endl;
        std::cout << shader.getInfoLog() << std::endl;
        std::cout << shader.getInfoDebugLog() << std::endl;
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages))
    {
        std::cout << "glslang_compile_glsl linking failed: " << std::endl;
        std::cout << program.getInfoLog() << std::endl;
        std::cout << program.getInfoDebugLog() << std::endl;
        return false;
    }

    spv::SpvBuildLogger spvLogger;
    glslang::SpvOptions options{
        .generateDebugInfo = true,
        .stripDebugInfo = false,
        .disableOptimizer = true,
        .optimizeSize = false,
    };
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirvCode, &spvLogger, &options);

    return true;
}

bool RShaderCompiler::compile_to_spirv(RShaderType type, const char* vkGLSL, std::vector<uint32_t>& spirvCode)
{
    const EShLanguage stage = get_glslang_shader_stage(type);
    const glslang::EShClient client = glslang::EShClientVulkan;
    const glslang::EshTargetClientVersion clientVersion = LD_GLSLANG_VULKAN_CLIENT_VERSION;

    return glslang_compile_glsl(client, clientVersion, stage, vkGLSL, spirvCode);
}

bool RShaderCompiler::compile_to_opengl_glsl(RShaderType type, const char* vkGLSL, std::string& glGLSL)
{
    const EShLanguage stage = get_glslang_shader_stage(type);
    const glslang::EShClient client = glslang::EShClientVulkan;
    const glslang::EshTargetClientVersion clientVersion = LD_GLSLANG_VULKAN_CLIENT_VERSION;

    std::vector<uint32_t> spirvCode;
    if (!glslang_compile_glsl(client, clientVersion, stage, vkGLSL, spirvCode))
        return false;

    spirv_cross::CompilerGLSL compiler(std::move(spirvCode));

    // TODO: remap layout qualifiers "set" + "binding" to OpenGL
    glGLSL = compiler.compile();

    return true;
}

} // namespace LD
