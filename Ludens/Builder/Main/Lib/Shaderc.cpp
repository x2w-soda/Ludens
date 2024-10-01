#include <iostream>
#include <sstream>
#include "Builder/Main/Lib/BuilderMain.h"
#include "Builder/Main/Lib/Shaderc.h"
#include "Core/CommandLine/Include/CommandLine.h"
#include "Core/RenderFX/Include/RShaderCompiler.h"

namespace LD {

Shaderc::Shaderc()
{
}

Shaderc::~Shaderc()
{
}

int Shaderc::Main(int argc, const char** argv)
{
    CommandLineArg argOpenGL;
    argOpenGL.FullName = "opengl";
    argOpenGL.Help = "compile spirv shaders for OpenGL backend";
    argOpenGL.IsFlag = true;

    CommandLineArg argVulkan;
    argVulkan.FullName = "vulkan";
    argVulkan.Help = "compile spirv shaders for Vulkan backend";
    argVulkan.IsFlag = true;

    CommandLineArg argOutput;
    argOutput.FullName = "output";
    argOutput.Help = "output directory for spirv shaders";

    CommandLineArg argInput;
    argInput.FullName = "input";
    argInput.Help = "one or more input shaders, written in Ludens GLSL";
    argInput.IsPositional = true;

    CommandLineParser parser;
    CommandLineResult result;
    int argOpenGLI = parser.AddArgument(argOpenGL);
    int argVulkanI = parser.AddArgument(argVulkan);
    int argOutputI = parser.AddArgument(argOutput);
    int argInputI = parser.AddArgument(argInput);

    result = parser.Parse(argc, argv);
    if (result.Type != CommandLineResultType::Ok)
    {
        std::cout << result.Error << std::endl;
        return 0;
    }

    std::string value;
    mOpenGL = parser.GetArgument(argOpenGLI, value);
    mVulkan = parser.GetArgument(argVulkanI, value);

    parser.GetArgument(argInputI, value);
    std::stringstream paths(value);
    PrintLn("input paths: %s", value.c_str());

    if (!parser.GetArgument(argOutputI, mOutputDir))
        mOutputDir = "./";
    PrintLn("output dir: %s", mOutputDir.c_str());
    
    while (std::getline(paths, value, ' '))
    {
        Path path(value);
        
        File sourceFile;
        bool result = sourceFile.Open(path, FileMode::Read);
        LD_DEBUG_ASSERT(result);

        const char* sourceData = (const char*)sourceFile.Data();
        size_t sourceSize = sourceFile.Size();

        if (mOpenGL)
        {
            PrintLn("compiling for OpenGL backend: %s", value.c_str());
            Compile(RBackend::OpenGL, path, sourceData, sourceSize);
        }
        if (mVulkan)
        {
            PrintLn("compiling for Vulkan backend: %s", value.c_str());
            Compile(RBackend::Vulkan, path, sourceData, sourceSize);
        }
    }

    return 0;
}

void Shaderc::Compile(RBackend target, const Path& inputPath, const char* data, size_t size)
{
    RShaderCompiler compiler(target);

    std::string glsl(data, size);
    Vector<RShaderCompileResult> results;
    compiler.Compile(glsl, results);

    PrintLn("number of compilations: %d", (int)results.Size());

    for (const RShaderCompileResult& result : results)
    {
        std::string fileName = mOutputDir + inputPath.Stem().ToString();
        fileName += target == RBackend::OpenGL ? "GL" : "VK";
        fileName += result.Type == RShaderType::VertexShader ? "VS" : "FS";
        fileName += ".spv";

        PrintLn("- target %s", fileName.c_str());

        if (!result.Success)
        {
            // dump error log in place of SPIRV
            fileName += ".txt";
            File logFile;
            logFile.Open({ fileName }, FileMode::Write);
            logFile.Write((const u8*)result.Error.data(), result.Error.size());
            logFile.Close();

            PrintLn("- compilation failure, error log at %s", fileName.c_str());
            continue;
        }

        PrintLn("- writing spirv (%d bytes) to %s", (int)result.SPIRV.Size(), fileName.c_str());

        File file;
        file.Open({ fileName }, FileMode::Write);
        file.Write(result.SPIRV.Data(), result.SPIRV.Size());
        file.Close();
    }

}

void Shaderc::Compile(RBackend target, const std::string& glsl, Vector<RShaderCompileResult>& results)
{
    RShaderCompiler compiler(target);
    compiler.Compile(glsl, results);
}

} // namespace LD