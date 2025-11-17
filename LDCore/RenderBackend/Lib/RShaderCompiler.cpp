#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <iostream>

// hide from module user
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "RShaderCompiler.h"

#define LD_GLSLANG_VULKAN_CLIENT_VERSION glslang::EShTargetVulkan_1_3
#define LD_GLSLANG_TARGET_SPIRV_VERSION glslang::EShTargetSpv_1_0

namespace LD {

static bool glslang_compile_glsl(glslang::EShClient client, glslang::EshTargetClientVersion clientVersion, EShLanguage stage, const char* glsl, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection);
static void glslang_reflect_spirv(const std::vector<uint32_t>& spirv, RShaderReflection& reflection);
static RShaderBinding glslang_reflect_binding(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource, RBindingType type);

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

static bool glslang_compile_glsl(glslang::EShClient client, glslang::EshTargetClientVersion clientVersion, EShLanguage stage, const char* glsl, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection)
{
    LD_PROFILE_SCOPE;

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

    if (reflection)
        glslang_reflect_spirv(spirvCode, *reflection);

    return true;
}

void glslang_reflect_spirv(const std::vector<uint32_t>& spirv, RShaderReflection& reflection)
{
    LD_PROFILE_SCOPE;

    spirv_cross::Compiler compiler(spirv);
    spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

    reflection.bindings.clear();

    for (const spirv_cross::Resource& resource : shaderResources.uniform_buffers)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_UNIFORM_BUFFER));

    for (const spirv_cross::Resource& resource : shaderResources.storage_buffers)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_STORAGE_BUFFER));

    for (const spirv_cross::Resource& resource : shaderResources.sampled_images)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER));

    for (const spirv_cross::Resource& resource : shaderResources.storage_images)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_STORAGE_IMAGE));
}

RShaderBinding glslang_reflect_binding(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource, RBindingType bindingType)
{
    const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);

    RShaderBinding binding;
    binding.setIndex = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    binding.bindingIndex = compiler.get_decoration(resource.id, spv::DecorationBinding);
    binding.arrayCount = type.array.empty() ? 1 : type.array[0];
    binding.name = resource.name;
    binding.type = bindingType;

    return binding;
}

bool RShaderCompiler::compile_to_spirv(RShaderType type, const char* vkGLSL, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection)
{
    LD_PROFILE_SCOPE;

    const EShLanguage stage = get_glslang_shader_stage(type);
    const glslang::EShClient client = glslang::EShClientVulkan;
    const glslang::EshTargetClientVersion clientVersion = LD_GLSLANG_VULKAN_CLIENT_VERSION;

    return glslang_compile_glsl(client, clientVersion, stage, vkGLSL, spirvCode, reflection);
}

bool RShaderCompiler::decompile_to_opengl_glsl(const std::vector<uint32_t>& spirvCode, std::string& glGLSL)
{
    spirv_cross::CompilerGLSL compiler(spirvCode);

    // TODO: remap layout qualifiers "set" + "binding" to OpenGL
    glGLSL = compiler.compile();

    return true;
}

} // namespace LD
