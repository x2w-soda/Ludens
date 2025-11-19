#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <iostream>

// hide from module user
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "RBackendObj.h"
#include "RShaderCompiler.h"

#define LD_GLSLANG_VULKAN_CLIENT_VERSION glslang::EShTargetVulkan_1_3
#define LD_GLSLANG_TARGET_SPIRV_VERSION glslang::EShTargetSpv_1_0

namespace LD {

static bool glslang_compile_glsl(glslang::EShClient client, glslang::EshTargetClientVersion clientVersion, EShLanguage stage, const char* glsl, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection);
static void glslang_reflect_spirv(const std::vector<uint32_t>& spirv, RShaderReflection& reflection);
static RShaderLocation glslang_reflect_location(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource);
static RShaderBinding glslang_reflect_binding(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource, RBindingType type);
static void cast_glsl_type(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& inType, GLSLType& outType);
static void cast_glsl_sampler_type(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& inType, GLSLType& outType);
static void cast_glsl_image_type(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& inType, GLSLType& outType);
static EShLanguage get_glslang_shader_stage(RShaderType type);
static void remap_vk_resource(const RShaderOpenGLRemap& remap, const spirv_cross::Resource& resource, spirv_cross::Compiler& compiler);

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

static void remap_vk_resource(const RShaderOpenGLRemap& remap, const spirv_cross::Resource& resource, spirv_cross::Compiler& compiler)
{
    uint32_t vkSetIndex = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    uint32_t vkBindingIndex = compiler.get_decoration(resource.id, spv::DecorationBinding);
    const auto* bindingRemap = remap.get_binding_remap(vkSetIndex, vkBindingIndex);
    LD_ASSERT(bindingRemap);

    compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
    compiler.set_decoration(resource.id, spv::DecorationBinding, bindingRemap->glBindingIndex);
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

static void glslang_reflect_spirv(const std::vector<uint32_t>& spirv, RShaderReflection& reflection)
{
    LD_PROFILE_SCOPE;

    spirv_cross::Compiler compiler(spirv);
    spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

    reflection.bindings.clear();

    for (const spirv_cross::Resource& resource : shaderResources.stage_inputs)
        reflection.inputs.emplace_back(glslang_reflect_location(compiler, resource));

    for (const spirv_cross::Resource& resource : shaderResources.stage_outputs)
        reflection.outputs.emplace_back(glslang_reflect_location(compiler, resource));

    for (const spirv_cross::Resource& resource : shaderResources.uniform_buffers)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_UNIFORM_BUFFER));

    for (const spirv_cross::Resource& resource : shaderResources.storage_buffers)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_STORAGE_BUFFER));

    for (const spirv_cross::Resource& resource : shaderResources.sampled_images)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER));

    for (const spirv_cross::Resource& resource : shaderResources.storage_images)
        reflection.bindings.emplace_back(glslang_reflect_binding(compiler, resource, RBINDING_TYPE_STORAGE_IMAGE));
}

static RShaderLocation glslang_reflect_location(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource)
{
    const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);

    GLSLType glslType;
    cast_glsl_type(compiler, type, glslType);

    RShaderLocation loc;
    loc.name = resource.name;
    loc.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
    loc.glslType = glslType;
    loc.arrayCount = type.array.empty() ? 1 : type.array[0];

    return loc;
}

static RShaderBinding glslang_reflect_binding(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource, RBindingType bindingType)
{
    const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);

    GLSLType glslType;
    cast_glsl_type(compiler, type, glslType);

    RShaderBinding binding;
    binding.setIndex = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    binding.bindingIndex = compiler.get_decoration(resource.id, spv::DecorationBinding);
    binding.arrayCount = type.array.empty() ? 1 : type.array[0];
    binding.name = resource.name;
    binding.type = bindingType;
    binding.glslType = glslType;

    return binding;
}

static void cast_glsl_type(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& inType, GLSLType& outType)
{
    if (inType.basetype == spirv_cross::SPIRType::Float)
    {
        switch (inType.vecsize)
        {
        case 1:
            outType = GLSL_TYPE_FLOAT;
            return;
        case 2:
            outType = GLSL_TYPE_VEC2;
            return;
        case 3:
            outType = GLSL_TYPE_VEC3;
            return;
        case 4:
            if (inType.columns == 4)
            {
                outType = GLSL_TYPE_MAT4;
                return;
            }
            outType = GLSL_TYPE_VEC4;
            return;
        }
    }
    else if (inType.basetype == spirv_cross::SPIRType::Double)
    {
        switch (inType.vecsize)
        {
        case 1:
            outType = GLSL_TYPE_DOUBLE;
            return;
        case 2:
            outType = GLSL_TYPE_DVEC2;
            return;
        case 3:
            outType = GLSL_TYPE_DVEC3;
            return;
        case 4:
            outType = GLSL_TYPE_DVEC4;
            return;
        }
    }
    else if (inType.basetype == spirv_cross::SPIRType::UInt)
    {
        switch (inType.vecsize)
        {
        case 1:
            outType = GLSL_TYPE_UINT;
            return;
        case 2:
            outType = GLSL_TYPE_UVEC2;
            return;
        case 3:
            outType = GLSL_TYPE_UVEC3;
            return;
        case 4:
            outType = GLSL_TYPE_UVEC4;
            return;
        }
    }
    else if (inType.basetype == spirv_cross::SPIRType::Struct)
    {
        outType = GLSL_TYPE_STRUCT;
        return;
    }
    else if (inType.basetype == spirv_cross::SPIRType::SampledImage)
    {
        cast_glsl_sampler_type(compiler, inType, outType);
        return;
    }
    else if (inType.basetype == spirv_cross::SPIRType::Image)
    {
        cast_glsl_image_type(compiler, inType, outType);
        return;
    }

    LD_UNREACHABLE;
}

static void cast_glsl_sampler_type(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& inType, GLSLType& outType)
{
    LD_ASSERT(inType.basetype == spirv_cross::SPIRType::SampledImage);
    LD_ASSERT(!inType.image.depth);

    const spirv_cross::SPIRType& imageType = compiler.get_type(inType.image.type);

    switch (imageType.basetype)
    {
    case spirv_cross::SPIRType::Float:
        switch (inType.image.dim)
        {
        case spv::Dim2D:
            outType = GLSL_TYPE_SAMPLER_2D;
            return;
        case spv::DimCube:
            outType = GLSL_TYPE_SAMPLER_CUBE;
            return;
        case spv::Dim1D: // TODO:
        case spv::Dim3D: // TODO:
        default:
            break;
        }
        break;
    case spirv_cross::SPIRType::UInt:
        switch (inType.image.dim)
        {
        case spv::Dim2D:
            outType = GLSL_TYPE_SAMPLER_2D;
            return;
        case spv::DimCube: // TODO:
        case spv::Dim1D:   // TODO:
        case spv::Dim3D:   // TODO:
        default:
            break;
        }
        break;
    case spirv_cross::SPIRType::Int: // TODO:
    default:
        break;
    }

    LD_UNREACHABLE;
}

static void cast_glsl_image_type(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& inType, GLSLType& outType)
{
    LD_ASSERT(inType.basetype == spirv_cross::SPIRType::Image);
    LD_ASSERT(!inType.image.depth);

    const spirv_cross::SPIRType& imageType = compiler.get_type(inType.image.type);

    switch (imageType.basetype)
    {
    case spirv_cross::SPIRType::UInt:
        switch (inType.image.dim)
        {
        case spv::Dim2D:
            outType = GLSL_TYPE_UIMAGE_2D;
            return;
        default:
            break;
        }
        break;
    case spirv_cross::SPIRType::Float: // TODO:
    case spirv_cross::SPIRType::Int:   // TODO:
    default:
        break;
    }

    LD_UNREACHABLE;
}

const RShaderOpenGLBindingRemap* RShaderOpenGLRemap::get_binding_remap(uint32_t vkSetIndex, uint32_t vkBindingIndex) const
{
    for (size_t i = 0; i < bindingRemaps.size(); i++)
    {
        const RShaderOpenGLBindingRemap* remap = bindingRemaps.data() + i;

        if (remap->vkSetIndex == vkSetIndex && remap->vkBindingIndex == vkBindingIndex)
            return remap;
    }

    return nullptr;
}

bool RShaderCompiler::compile_to_spirv(RShaderType type, const char* vkGLSL, std::vector<uint32_t>& spirvCode, RShaderReflection* reflection)
{
    LD_PROFILE_SCOPE;

    const EShLanguage stage = get_glslang_shader_stage(type);
    const glslang::EShClient client = glslang::EShClientVulkan;
    const glslang::EshTargetClientVersion clientVersion = LD_GLSLANG_VULKAN_CLIENT_VERSION;

    return glslang_compile_glsl(client, clientVersion, stage, vkGLSL, spirvCode, reflection);
}

bool RShaderCompiler::compute_opengl_remap(const RPipelineLayoutObj* layoutObj, RShaderOpenGLRemap& remap)
{
    remap.bindingRemaps.clear();

    uint32_t uboBindingCtr = 0;
    uint32_t ssboBindingCtr = 0;
    uint32_t sampledImageBindingCtr = 0;
    uint32_t storageImageBindingCtr = 0;

    for (uint32_t setIdx = 0; setIdx < layoutObj->setCount; setIdx++)
    {
        RSetLayoutObj* setLayoutObj = layoutObj->setLayoutObjs[setIdx];

        for (uint32_t bindingIdx = 0; bindingIdx < (uint32_t)setLayoutObj->bindings.size(); bindingIdx++)
        {
            const RSetBindingInfo& bindingI = setLayoutObj->bindings[bindingIdx];
            RShaderOpenGLBindingRemap bindingRemap;
            bindingRemap.vkBindingIndex = bindingIdx;
            bindingRemap.vkSetIndex = setIdx;

            switch (bindingI.type)
            {
            case RBINDING_TYPE_COMBINED_IMAGE_SAMPLER:
                bindingRemap.glBindingIndex = sampledImageBindingCtr;
                sampledImageBindingCtr += bindingI.arrayCount;
                break;
            case RBINDING_TYPE_STORAGE_IMAGE:
                bindingRemap.glBindingIndex = storageImageBindingCtr;
                storageImageBindingCtr += bindingI.arrayCount;
                break;
            case RBINDING_TYPE_UNIFORM_BUFFER:
                bindingRemap.glBindingIndex = uboBindingCtr;
                uboBindingCtr += bindingI.arrayCount;
                break;
            case RBINDING_TYPE_STORAGE_BUFFER:
                bindingRemap.glBindingIndex = ssboBindingCtr;
                ssboBindingCtr += bindingI.arrayCount;
                break;
            default:
                LD_UNREACHABLE;
            }

            remap.bindingRemaps.push_back(bindingRemap);
        }
    }

    return true;
}

bool RShaderCompiler::decompile_to_opengl_glsl(const RShaderOpenGLRemap& remap, const std::vector<uint32_t>& spirvCode, std::string& glGLSL)
{
    spirv_cross::CompilerGLSL compiler(spirvCode);
    spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

    // Only 4 types in GLSL require binding remap
    // 1. UBO
    // 2. SSBO
    // 3. Sampled Image (Combined Image Sampler)
    // 4. Storage Image

    for (const spirv_cross::Resource& resource : shaderResources.uniform_buffers)
        remap_vk_resource(remap, resource, compiler);

    for (const spirv_cross::Resource& resource : shaderResources.storage_buffers)
        remap_vk_resource(remap, resource, compiler);

    for (const spirv_cross::Resource& resource : shaderResources.sampled_images)
        remap_vk_resource(remap, resource, compiler);

    for (const spirv_cross::Resource& resource : shaderResources.storage_images)
        remap_vk_resource(remap, resource, compiler);

    glGLSL = compiler.compile();

    return true;
}

} // namespace LD
