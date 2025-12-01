#include <Ludens/CommandLine/ArgParser.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Platform.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/System/FileSystem.h>
#include <LudensBuilder/BAssetUtil/BAssetUtil.h>
#include <LudensBuilder/BDocumentCompiler/BDocumentCompiler.h>

#include <array>
#include <cstdlib>
#include <cstring>

#include "AudioUtil.h"
#include "MeshUtil.h"
#include "RenderUtil.h"
#include "RunTests.h"

using namespace LD;

namespace LD {

static void print_help(const char* argv0);
static int find_argi(int argc, char** argv, const char* match);
static void builder_mode_import(int argc, char** argv);
static void builder_mode_render(int argc, char** argv);
static void builder_mode_run_tests(int argc, char** argv);

static Log sLog("LDBuilder");

enum BuilderMode
{
    BUILDER_MODE_ERROR = 0,
    BUILDER_MODE_IMPORT,
    BUILDER_MODE_RENDER,
    BUILDER_MODE_RUN_TESTS,
};

class BuilderArgs
{
public:
    BuilderArgs() = delete;
    BuilderArgs(int argc, char** argv);
    BuilderArgs(const BuilderArgs&) = delete;
    ~BuilderArgs();

    BuilderArgs& operator=(const BuilderArgs&) = delete;

    inline BuilderMode get_mode() const { return mMode; }

private:
    ArgParser mParser;
    BuilderMode mMode = BUILDER_MODE_ERROR;
    int argvIndex = 0;
};

BuilderArgs::BuilderArgs(int argc, char** argv)
{
    std::array<ArgOption, 1> options;
    options[0] = {
        .index = 0,
        .shortName = "h",
        .longName = "help",
        .payload = ARG_PAYLOAD_NONE,
    };

    mParser = ArgParser::create((int)options.size(), options.data());
    mParser.parse(argc - 1, (const char**)argv + 1);

    int optIndex, errIndex;
    const char* optPayload;
    while ((optIndex = mParser.getopt(&optPayload, errIndex)))
    {
        if (optIndex < 0 && ((ArgResult)optIndex == ARG_RESULT_EOF))
            break;

        // first positional argument decides builder mode.
        if (optIndex < 0 && ((ArgResult)optIndex == ARG_RESULT_POSITIONAL))
        {
            if (!strcmp(optPayload, "import"))
            {
                mMode = BUILDER_MODE_IMPORT;
                break;
            }
            else if (!strcmp(optPayload, "render"))
            {
                mMode = BUILDER_MODE_RENDER;
                break;
            }
            else if (!strcmp(optPayload, "run_tests"))
            {
                mMode = BUILDER_MODE_RUN_TESTS;
                break;
            }
        }
    }
}

BuilderArgs::~BuilderArgs()
{
    ArgParser::destroy(mParser);
}

static void print_help(const char* argv0)
{
    sLog.info("usage: {} [options]? [mode]", argv0);
    sLog.info("  options:");
    sLog.info("    -h --help: print this help menu and exit");
    sLog.info("  mode:");
    sLog.info("    import: asset import utilities");
    sLog.info("    render: offline rendering utilities");
}

static int find_argi(int argc, char** argv, const char* match)
{
    int argi = 0;

    while (argi < argc && strcmp(argv[argi], match))
        argi++;

    return argi;
}

static void builder_mode_import(int argc, char** argv)
{
    int argi = find_argi(argc, argv, "import");
    if (argi >= argc)
    {
        LD_UNREACHABLE; // caller is gaslighting
        return;
    }
    argv += argi;
    argc -= argi;

    if (argc != 3)
    {
        // TODO: help message for import mode
        sLog.info("import mode invalid args");
        return;
    }

    AssetUtil util = AssetUtil::create();
    bool success = false;

    std::string type(argv[1]);
    FS::Path sourcePath(argv[2]);

    if (type == "Texture2D")
        success = util.import_texture_2d(sourcePath);
    else if (type == "Font")
        success = util.import_font(sourcePath);
    else if (type == "Mesh")
        success = util.import_mesh(sourcePath);
    else if (type == "AudioClip")
        success = util.import_audio_clip(sourcePath);

    if (!success)
        sLog.warn("import failed");

    AssetUtil::destroy(util);
}

static void builder_mode_render(int argc, char** argv)
{
    int argi = find_argi(argc, argv, "render");
    if (argi >= argc)
    {
        LD_UNREACHABLE; // caller is gaslighting
        return;
    }
    argv += argi;
    argc -= argi;

    if (argc != 3 || std::string(argv[1]) != "env_to_faces")
    {
        // TODO: help message for render mode
        sLog.info("render mode invalid args");
        return;
    }

    FS::Path inputPath = FS::Path(argv[2]);
    if (!FS::exists(inputPath))
    {
        sLog.error("input path {} does not exist", inputPath.string());
        return;
    }

    FS::Path dirPath = FS::Path(inputPath).remove_filename();
    RenderUtil util = RenderUtil::create();
    {
        util.from_equirectangular_to_faces(inputPath, dirPath);
    }
    RenderUtil::destroy(util);
}

static void builder_mode_run_tests(int argc, char** argv)
{
    int argi = find_argi(argc, argv, "run_tests");
    if (argi >= argc)
    {
        LD_UNREACHABLE; // caller is gaslighting
        return;
    }
    argv += argi;
    argc -= argi;

    if (argc != 2)
    {
        // TODO: help message for run_tests mode
        sLog.info("run_tests mode invalid args");
        return;
    }

    FS::Path dirPath = FS::Path(argv[1]);

    if (!FS::is_directory(dirPath))
    {
        sLog.error("directory {} does not exist", dirPath.string());
        return;
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
        return;

    int passCount = run_test_exectuables(testPaths);
    sLog.info("{}/{} tests passed", passCount, testCount);
}

} // namespace LD

int main(int argc, char** argv)
{
    sLog.info("PWD: {}", std::filesystem::current_path().string());

    BuilderArgs args(argc, argv);

    JobSystemInfo systemI{};
    systemI.immediateQueueCapacity = 512;
    systemI.standardQueueCapacity = 512;
    JobSystem::init(systemI);

    switch (args.get_mode())
    {
    case BUILDER_MODE_IMPORT:
        builder_mode_import(argc, argv);
        break;
    case BUILDER_MODE_RENDER:
        builder_mode_render(argc, argv);
        break;
    case BUILDER_MODE_RUN_TESTS:
        builder_mode_run_tests(argc, argv);
        break;
    case BUILDER_MODE_ERROR:
    default:
        print_help(argv[0]);
        return EXIT_FAILURE;
        break;
    }

    JobSystem::shutdown();

    return EXIT_SUCCESS;
}