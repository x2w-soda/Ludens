#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_glsl.hpp>
#include "Core/Header/Include/Error.h"
#include "Core/Header/Include/Types.h"
#include "Core/OS/Include/Exit.h"
#include "Core/DSA/Include/Optional.h"
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderFX/Include/RShaderCompiler.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ToneMappingGroup.h"
#include "Core/RenderFX/Include/Groups/CubemapGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/Groups/MaterialGroup.h"
#include "Core/RenderFX/Include/Groups/RectGroup.h"
#include "Core/RenderFX/Include/Groups/SSAOGroup.h"

#define LD_MACRO_INVALID -1
#define LD_MACRO_GROUP_LAYOUT 1
#define LD_MACRO_SHADER_STAGE 2

namespace LD
{

struct Qualifier
{
    u32 Group;
    u32 Binding;

    bool operator==(const Qualifier& other) const
    {
        return Group == other.Group && Binding == other.Binding;
    }

    bool operator<(const Qualifier& other) const
    {
        if (Group != other.Group)
            return Group < other.Group;

        return Binding < other.Binding;
    }
};

RShaderCompiler::RShaderCompiler(RBackend backend) : mTargetBackend(backend)
{
    static bool sIsFirstInstance = true;

    if (sIsFirstInstance)
    {
        sIsFirstInstance = false;

        glslang::InitializeProcess();

        AtExit([]() { glslang::FinalizeProcess(); });
    }
}

RShaderCompiler::~RShaderCompiler()
{
}

void RShaderCompiler::Compile(const std::string& glsl, Vector<RShaderCompileResult>& results)
{
    results.Clear();

    RPipelineLayoutData layout;
    RShaderType nextShaderType;
    std::stringstream ss(glsl);
    std::string line, shaderGLSL;
    Optional<RShaderType> pending;

    while (std::getline(ss, line))
    {
        int macro = ParseLudensMacro(line, layout, nextShaderType);

        // invalid macro
        if (macro < 0)
        {
            std::cout << "invalid #ludens macro" << std::endl;
            return;
        }

        // describing binding group layout
        if (macro == LD_MACRO_GROUP_LAYOUT)
            continue;

        // shader stage marker
        if (macro == LD_MACRO_SHADER_STAGE)
        {
            if (pending.HasValue())
            {
                RShaderCompileResult& result = results.Back();
                result.Type = pending.Value();
                std::string typeStr = (result.Type == RShaderType::VertexShader ? "VS" : "FS");
                CompileStage(layout, pending.Value(), shaderGLSL, result);
                pending = nextShaderType;
            }

            results.PushBack({});
            shaderGLSL.clear();
            pending = nextShaderType;
            continue;
        }

        // accumulate shader source
        shaderGLSL += line + '\n';
    }

    if (pending.HasValue())
    {
        RShaderCompileResult& result = results.Back();
        result.Type = pending.Value();
        std::string typeStr = (result.Type == RShaderType::VertexShader ? "VS" : "FS");
        CompileStage(layout, pending.Value(), shaderGLSL, result);
    }
}

void RShaderCompiler::CompileStage(const RPipelineLayoutData& layout, RShaderType type, const std::string& glsl,
                                   RShaderCompileResult& result)
{
    Vector<u32> spirv_u32;
    std::string input_glsl(glsl);
    std::string preamble;

    if (mTargetBackend == RBackend::OpenGL)
    {
        PatchOpenGL(layout, type, input_glsl);
        preamble = "#define LD_OPENGL\n";

        //std::cout << "PATCHED OPENGL BEGIN" << std::endl;
        //std::cout << input_glsl << std::endl;
        //std::cout << "PATCHED OPENGL END" << std::endl;
    }
    else if (mTargetBackend == RBackend::Vulkan)
        preamble = "#define LD_VULKAN\n";

    result.Success = GlslangCompile(mTargetBackend, type, input_glsl, preamble, spirv_u32, result.Error);

    result.SPIRV.Resize(spirv_u32.Size() * 4);
    for (size_t i = 0; i < spirv_u32.Size(); i++)
    {
        result.SPIRV[4 * i + 0] = spirv_u32[i] & 0xFF;
        result.SPIRV[4 * i + 1] = (spirv_u32[i] >> 8) & 0xFF;
        result.SPIRV[4 * i + 2] = (spirv_u32[i] >> 16) & 0xFF;
        result.SPIRV[4 * i + 3] = (spirv_u32[i] >> 24) & 0xFF;
    }
}

void RShaderCompiler::GlslangShaderType(RShaderType type, EShLanguage* planguage)
{
    EShLanguage language;

    switch (type)
    {
    case RShaderType::VertexShader:
        language = EShLangVertex;
        break;
    case RShaderType::FragmentShader:
        language = EShLangFragment;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }

    if (planguage)
        *planguage = language;
}

void RShaderCompiler::GlslangBackend(RBackend backend, glslang::EShClient* pclient,
                                     glslang::EShTargetClientVersion* pversion)
{
    glslang::EShClient client;
    glslang::EShTargetClientVersion version;

    switch (backend)
    {
    case RBackend::OpenGL:
        client = glslang::EShClientOpenGL;
        version = glslang::EShTargetOpenGL_450;
        break;
    case RBackend::Vulkan:
        client = glslang::EShClientVulkan;
        version = glslang::EShTargetVulkan_1_3;
        break;
    }

    if (pclient)
        *pclient = client;
    if (*pversion)
        *pversion = version;
}

bool RShaderCompiler::GlslangCompile(RBackend backend, RShaderType type, const std::string& glsl,
                                     const std::string& glslPreamble, Vector<u32>& spirv, std::string& error)
{
    EShLanguage shaderType;
    glslang::EShClient client;
    glslang::EShTargetClientVersion clientVersion;

    GlslangShaderType(type, &shaderType);
    GlslangBackend(backend, &client, &clientVersion);

    std::string preamble = glslPreamble + std::string("#define group set\n");

    glslang::TShader shader(shaderType);
    {
        const char* glslSource = glsl.c_str();
        shader.setStrings(&glslSource, 1);
        shader.setEntryPoint("main");
        shader.setPreamble(preamble.c_str());
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
        shader.setEnvClient(client, clientVersion);

        bool result = shader.parse(GetDefaultResources(), 100, false, EShMsgDefault);
        const char* log;

        if (!result)
        {
            if ((log = shader.getInfoLog()) && strlen(log) > 0)
            {
                std::string line("glslang::TShader::getInfoLog(): ");
                line += log;
                std::cout << line << std::endl;
                error += line;
            }

            if ((log = shader.getInfoDebugLog()) && strlen(log) > 0)
            {
                std::string line("glslang::TShader::getInfoDebugLog(): ");
                line += log;
                std::cout << line << std::endl;
                error += line;
            }

            return false;
        }
    }

    glslang::TProgram program{};
    {
        program.addShader(&shader);

        bool result = program.link(EShMsgDefault);

        if (!result)
            return false;
    }

    // NOTE: glslang API uses std::vector<unsigned int> for spirv, but unsigned int may not be 32-bit words on all
    // platforms
    std::vector<unsigned int> spirv_ui;
    glslang::TIntermediate* ir = program.getIntermediate(shaderType);
    glslang::SpvOptions options{};
    glslang::GlslangToSpv(*ir, spirv_ui, &options);

    spirv.Resize(spirv_ui.size());
    for (size_t i = 0; i < spirv_ui.size(); i++)
    {
        spirv[i] = (u32)spirv_ui[i];
    }

    return true;
}

void RShaderCompiler::PatchOpenGL(const RPipelineLayoutData& layout, RShaderType type, std::string& glsl)
{
    // converts RenderBase canonical GLSL into OpenGL compatible GLSL
    // we first compile Vulkan GLSL to SPIRV, then patch and reflect SPIRV back to OpenGL GLSL

    size_t groupCount = layout.GroupLayouts.Size();
    u32 textureUnitCtr = 0;
    u32 uboBaseCtr = 0;
    std::map<Qualifier, u32> textureUnitRemap;
    std::map<Qualifier, u32> uboBaseRemap;

    for (size_t groupIdx = 0; groupIdx < groupCount; groupIdx++)
    {
        auto& groupBindings = layout.GroupLayouts[groupIdx];
        size_t bindingCount = groupBindings.Size();

        for (size_t bindingIdx = 0; bindingIdx < bindingCount; bindingIdx++)
        {
            const RBindingInfo& bindingInfo = groupBindings[bindingIdx];

            switch (bindingInfo.Type)
            {
            case RBindingType::Texture:
                textureUnitRemap[{ (u32)groupIdx, (u32)bindingIdx }] = textureUnitCtr;
                textureUnitCtr +=
                    bindingInfo.Count; // an array of samplers uses one texture unit for each sampled texture
                break;
            case RBindingType::UniformBuffer:
                uboBaseRemap[{ (u32)groupIdx, (u32)bindingIdx }] = uboBaseCtr++;
                break;
            }
        }
    }

    // even though ludens source GLSL is using the Vulkan dialect, we will still reconstruct
    // OpenGL GLSL later, so we respect the LD_OPENGL directives in the source GLSL
    std::string preamble = "#define LD_OPENGL\n";
    std::string patchError;

    Vector<u32> spirv;
    bool ok = GlslangCompile(RBackend::Vulkan, type, glsl, preamble, spirv, patchError);
    LD_DEBUG_ASSERT(ok && "PatchOpenGL failed to compile Ludens GLSL to SPIRV");

    // TODO: validate shader reflection result with the pipeline layout, find any conflicts
    try
    {
        spirv_cross::CompilerGLSL compiler(spirv.Data(), spirv.Size());
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        for (auto& uniform_buffer : resources.uniform_buffers)
        {
            u32 id = uniform_buffer.id;
            u32 group = compiler.get_decoration(id, spv::DecorationDescriptorSet);
            u32 binding = compiler.get_decoration(id, spv::DecorationBinding);
            compiler.unset_decoration(id, spv::DecorationDescriptorSet);
            compiler.unset_decoration(id, spv::DecorationBinding);

            u32 uboBase = uboBaseRemap[{ group, binding }];
            compiler.set_decoration(id, spv::DecorationBinding, uboBase);
        }

        for (auto& sampled_image : resources.sampled_images)
        {
            u32 id = sampled_image.id;
            u32 group = compiler.get_decoration(id, spv::DecorationDescriptorSet);
            u32 binding = compiler.get_decoration(id, spv::DecorationBinding);
            compiler.unset_decoration(id, spv::DecorationDescriptorSet);
            compiler.unset_decoration(id, spv::DecorationBinding);

            LD_DEBUG_ASSERT(textureUnitRemap.find({ group, binding }) != textureUnitRemap.end());
            u32 textureUnit = textureUnitRemap[{ group, binding }];
            compiler.set_decoration(id, spv::DecorationBinding, textureUnit);
        }

        // output OpenGL compatible GLSL
        glsl = compiler.compile();
    }
    catch (spirv_cross::CompilerError error)
    {
        std::cout << "spirv_cross::CompilerError " << error.what() << std::endl;
    };
}

// TODO: macro parsing is super rough and dirty with zero tolerance, smooth this out
int RShaderCompiler::ParseLudensMacro(const std::string& line, RPipelineLayoutData& layout, RShaderType& type)
{
    const char* str = line.c_str();
    const char* macroStr = "#ludens";
    const char* countStr = "count";
    const char* groupStr = "group";
    const char* bindingStr = "binding";
    const char* uniformBufferStr = "UniformBuffer";
    const char* textureStr = "Texture";
    const char* vertexStr = "vertex";
    const char* fragmentStr = "fragment";
    int groupIdx, bindingIdx, bindingCnt;

    // return 0 if the input line is not a ludens macro
    if (strncmp(str, macroStr, strlen(macroStr)) != 0)
        return 0;

    str += strlen(macroStr) + 1;

    if (strncmp(str, groupStr, strlen(groupStr)) == 0)
    {
        str += strlen(groupStr) + 1;
        groupIdx = *str - '0';
        str += 2;

        LD_DEBUG_ASSERT(0 <= groupIdx && groupIdx < 4);

        if (layout.GroupLayouts.Size() < groupIdx + 1)
            layout.GroupLayouts.Resize(groupIdx + 1);

        RBindingGroupLayoutData& layoutData = layout.GroupLayouts[groupIdx];

        // use prefab binding group defined in RenderFX module
        if (strncmp(str, bindingStr, strlen(bindingStr)) != 0)
        {
            return ParseLudensMacroGroupPrefab(str, layoutData);
        }

        str += strlen(bindingStr) + 1;
        bindingIdx = *str - '0';
        str += 2;

        if (layoutData.Size() < bindingIdx + 1)
            layoutData.Resize(bindingIdx + 1);

        LD_DEBUG_ASSERT(strncmp(str, countStr, strlen(countStr)) == 0);
        str += strlen(countStr) + 1;

        bindingCnt = 0;
        while (*str && '0' <= *str && *str <= '9')
        {
            bindingCnt = bindingCnt * 10 + (*str - '0');
            str++;
        }
        str++;

        LD_DEBUG_ASSERT(0 < bindingCnt && bindingCnt <= 32);

        layoutData[bindingIdx].Count = bindingCnt;

        if (strncmp(str, uniformBufferStr, strlen(uniformBufferStr)) == 0)
            layoutData[bindingIdx].Type = RBindingType::UniformBuffer;
        else if (strncmp(str, textureStr, strlen(textureStr)) == 0)
            layoutData[bindingIdx].Type = RBindingType::Texture;
        else
            return LD_MACRO_INVALID;

        return LD_MACRO_GROUP_LAYOUT;
    }

    if (strncmp(str, vertexStr, strlen(vertexStr)) == 0)
    {
        type = RShaderType::VertexShader;
        return LD_MACRO_SHADER_STAGE;
    }
    else if (strncmp(str, fragmentStr, strlen(fragmentStr)) == 0)
    {
        type = RShaderType::FragmentShader;
        return LD_MACRO_SHADER_STAGE;
    }

    // invalid ludens macro
    return -1;
}

int RShaderCompiler::ParseLudensMacroGroupPrefab(std::string str, RBindingGroupLayoutData& layoutData)
{
    while (!str.empty() && isspace(str[str.size() - 1]))
        str.pop_back();

    if (str == "FrameStatic")
        layoutData = FrameStaticGroup{}.GetLayoutData();
    else if (str == "ToneMapping")
        layoutData = ToneMappingGroup{}.GetLayoutData();
    else if (str == "Cubemap")
        layoutData = CubemapGroup{}.GetLayoutData();
    else if (str == "Viewport")
        layoutData = ViewportGroup{}.GetLayoutData();
    else if (str == "Material")
        layoutData = MaterialGroup{}.GetLayoutData();
    else if (str == "Rect")
        layoutData = RectGroup{}.GetLayoutData();
    else if (str == "SSAO")
        layoutData = SSAOGroup{}.GetLayoutData();
    else
    {
        std::cout << "unknown prefab binding group [" << str << "]" << std::endl;
        return LD_MACRO_INVALID;
    }

    return LD_MACRO_GROUP_LAYOUT;
}

RShaderCache::~RShaderCache()
{
    LD_DEBUG_ASSERT(!mHasStartup && "RShaders not destroyed yet");
}

void RShaderCache::Startup(const RShaderCacheInfo& info)
{
    LD_DEBUG_ASSERT((bool)info.Device && "invalid device");

    mDevice = info.Device;
    mCacheDirectory = info.CacheDirectory;
    mHasStartup = true;
}

void RShaderCache::Cleanup()
{
    mHasStartup = false;

    for (auto& kv : mCacheInfo)
    {
        RResult result = mDevice.DeleteShader(kv.second.Shader);
        LD_DEBUG_ASSERT(result.Type == RResultType::Ok);
    }
}

RResult RShaderCache::GetOrCreateShader(const RPipelineLayout& layout, RShaderType type, const std::string& glsl,
                                        std::string name, RShader& shader)
{
    name = (mDevice.GetBackend() == RBackend::Vulkan ? "vk_" : "gl_") + name;
    size_t sourceHash = std::hash<std::string>{}(glsl);
    std::string sourceHashStr = std::to_string(sourceHash);
    std::string basePath{ mCacheDirectory.ToString() + "/" + name };
    Path spirvPath{ basePath + ".spv" };
    Path hashPath{ basePath + ".txt" };
    Vector<u8> spirv;
    RResult result;
    File file;

    if (!File::Exists(mCacheDirectory))
    {
        FileSystem fs;
        fs.CreateDirectories(mCacheDirectory);
    }

    // load from memory
    if (mCacheInfo.find(name) != mCacheInfo.end())
    {
        CacheInfo& ci = mCacheInfo[name];

        if (ci.ShaderType == type && ci.SourceHash == sourceHash)
            shader = ci.Shader;

        return {};
    }

    bool spirvFromDisk = false;

    if (File::Exists(spirvPath) && File::Exists(hashPath))
    {
        std::string diskHashStr;

        file.Open(hashPath, FileMode::Read);
        diskHashStr.resize(file.Size());
        memcpy(diskHashStr.data(), file.Data(), file.Size());
        file.Close();

        if (diskHashStr == sourceHashStr)
        {
            file.Open(spirvPath, FileMode::Read);
            spirv.Resize(file.Size());
            memcpy(spirv.Data(), file.Data(), file.Size());
            file.Close();

            spirvFromDisk = true;
        }
    }

    RShaderCompileResult compileResult;

    if (!spirvFromDisk) // compile glsl for target backend and save spirv to disk
    {
        RPipelineLayoutData layoutData;
        RShaderCompiler compiler(mDevice.GetBackend());
        layout.ToData(layoutData);

        compiler.CompileStage(layoutData, type, glsl, compileResult);

        File file;
        file.Open(spirvPath, FileMode::Write);
        file.Write(compileResult.SPIRV.Data(), compileResult.SPIRV.Size());
        file.Close();
        file.Open(hashPath, FileMode::Write);
        file.Write((const u8*)sourceHashStr.data(), sourceHashStr.size());
        file.Close();
    }

    RShaderInfo info{};
    info.SourceType = RShaderSourceType::SPIRV;
    info.Type = type;
    info.Data = spirvFromDisk ? spirv.Data() : compileResult.SPIRV.Data();
    info.Size = spirvFromDisk ? spirv.Size() : compileResult.SPIRV.Size();
    result = mDevice.CreateShader(shader, info);

    if (result.Type == RResultType::Ok)
    {
        mCacheInfo[name].ShaderType = type;
        mCacheInfo[name].Shader = shader;
        mCacheInfo[name].SourceHash = sourceHash;
    }

    return result;
}

} // namespace LD