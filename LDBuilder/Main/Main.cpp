#include "AudioUtil.h"
#include "MeshUtil.h"
#include "RenderUtil.h"
#include "RunTests.h"
#include <Ludens/Header/Platform.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/System/FileSystem.h>
#include <LudensBuilder/BAssetUtil/BAssetUtil.h>
#include <LudensBuilder/BDocumentCompiler/BDocumentCompiler.h>
#include <cstdlib>
#include <filesystem>

namespace LD {

static Log sLog("LDBuilder");

} // namespace LD

using namespace LD;
namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    sLog.info("PWD: {}", fs::current_path().string());

    // TODO: use command line parser from LDCommandLine module
    if (argc < 3)
    {
        sLog.info("usage: {} [options] [inputs]", argv[0]);
        sLog.info("  options:");
        sLog.info("    run_tests [path_to_directory]");
        sLog.info("    env_to_faces [path_to_env_map]");
        sLog.info("    extract_mesh_vertex [path_to_3d_model]");
        return EXIT_FAILURE;
    }

    JobSystemInfo systemI{};
    systemI.immediateQueueCapacity = 512;
    systemI.standardQueueCapacity = 512;
    JobSystem::init(systemI);

    std::string mode(argv[1]);

    if (mode == "env_to_faces")
    {
        fs::path inputPath = fs::path(argv[2]);
        if (!fs::exists(inputPath))
        {
            sLog.error("input path {} does not exist", inputPath.string());
            return EXIT_FAILURE;
        }

        fs::path dirPath = fs::path(inputPath).remove_filename();
        RenderUtil util = RenderUtil::create();
        {
            util.from_equirectangular_to_faces(inputPath, dirPath);
        }
        RenderUtil::destroy(util);
    }
    else if (mode == "run_tests")
    {
        fs::path dirPath = fs::path(argv[2]);
        if (!fs::is_directory(dirPath))
        {
            sLog.error("directory {} does not exist", dirPath.string());
            return EXIT_FAILURE;
        }

        const char* executableExt = nullptr;
#ifdef LD_PLATFORM_WIN32
        executableExt = ".exe";
#endif

        std::vector<std::string> testPaths;
        find_test_executables(dirPath.string().c_str(), testPaths, executableExt);

        sLog.info("found {} test executables:", (int)testPaths.size());
        for (const std::string& testPath : testPaths)
            sLog.info("  {}", testPath);

        int testCount = testPaths.size();
        if (testCount == 0)
            return EXIT_SUCCESS;

        int passCount = run_test_exectuables(testPaths);
        sLog.info("{}/{} tests passed", passCount, testCount);
    }
    else if (false && mode == "resample") // TODO: API to write WAV to disk. This path is closed for now.
    {
        AudioUtil util = AudioUtil::create();
        const fs::path inputPath = fs::path(argv[2]);

        const uint32_t dstSampleRate = 44100;

        std::string fileExt = inputPath.extension().string();
        std::string fileName = fs::path(inputPath).replace_extension("").string();
        fileName += "_44100";
        fileName += fileExt;
        fs::path outputPath(fileName);
        bool success = util.resample(inputPath, outputPath, dstSampleRate, SAMPLE_FORMAT_F32);

        if (!success)
            sLog.warn("resampling failed");

        AudioUtil::destroy(util);
    }
    else if (mode == "extract_mesh_vertex")
    {
        MeshUtil util = MeshUtil::create();
        const fs::path inputPath = fs::path(argv[2]);

        bool success = util.extract_mesh_vertex(inputPath);

        if (!success)
            sLog.warn("extraction failed");

        MeshUtil::destroy(util);
    }
    else if (mode == "compile_documents")
    {
        DocumentCompilerInfo compilerI{};
        compilerI.pathToDoxygenXML = fs::path(argv[2]);
        DocumentCompiler compiler = DocumentCompiler::create(compilerI);
        DocumentCompiler::destroy(compiler);
    }
    else if (mode == "import")
    {
        AssetUtil util = AssetUtil::create();

        bool success = false;

        std::string type(argv[2]);
        fs::path sourcePath(argv[3]);

        if (type == "Texture2D")
            success = util.import_texture_2d(sourcePath);
        else if (type == "Mesh")
            success = util.import_mesh(sourcePath);

        if (!success)
            sLog.warn("import failed");

        AssetUtil::destroy(util);
    }

    JobSystem::shutdown();

    return EXIT_SUCCESS;
}