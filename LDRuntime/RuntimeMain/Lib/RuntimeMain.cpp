#include <Ludens/CommandLine/ArgParser.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/FileSystem.h>

#include <array>
#include <format>

#include "RuntimeApplication.h"

#define ARGV_PROJECT_SCHEMA_PATH 0

using namespace LD;

namespace LD {

static Log sLog("Runtime");

class RuntimeArgs
{
public:
    RuntimeArgs() = delete;
    RuntimeArgs(int argc, char** argv);
    RuntimeArgs(const RuntimeArgs&) = delete;
    ~RuntimeArgs();

    inline FS::Path get_project_schema_path() const
    {
        return mProjectSchemaPath;
    }

    RuntimeArgs& operator=(const RuntimeArgs&) = delete;

private:
    ArgParser mParser;
    FS::Path mProjectSchemaPath;
};

RuntimeArgs::RuntimeArgs(int argc, char** argv)
    : mProjectSchemaPath("./project.toml")
{
    std::array<ArgOption, 1> options;
    options[0] = {
        .index = ARGV_PROJECT_SCHEMA_PATH,
        .shortName = "p",
        .longName = "project",
        .payload = ARG_PAYLOAD_REQUIRED,
    };

    mParser = ArgParser::create((int)options.size(), options.data());
    mParser.parse(argc - 1, (const char**)argv + 1);

    int optIndex, errIndex;
    const char* optPayload = nullptr;
    do
    {
        optIndex = mParser.getopt(&optPayload, errIndex);

        switch (optIndex)
        {
        case ARGV_PROJECT_SCHEMA_PATH:
            mProjectSchemaPath = FS::Path(optPayload).lexically_normal();
            break;
        default:
            break;
        }
    } while ((ArgResult)optIndex != ARG_RESULT_EOF);
}

RuntimeArgs::~RuntimeArgs()
{
    ArgParser::destroy(mParser);
}

} // namespace LD

int main(int argc, char** argv)
{
    FS::Path pwd = std::filesystem::current_path();
    sLog.info("PWD: {}", pwd.string());

    RuntimeArgs args(argc, argv);

    FS::Path projectSchemaPath = FS::Path(pwd / args.get_project_schema_path()).lexically_normal();
    if (!FS::exists(projectSchemaPath))
    {
        sLog.info("project schema path [{}] not found", projectSchemaPath.string());
        return 0;
    }

    FS::Path projectRootPath = projectSchemaPath.parent_path();
    Project project = Project::create(projectRootPath);
    ProjectSchema::load_project_from_file(project, projectSchemaPath);

    {
        RuntimeApplication runtimeApp(project);
        runtimeApp.run();
    }

    Project::destroy(project);

    int count = get_memory_leaks(nullptr);

    if (count > 0)
    {
        std::vector<MemoryProfile> leaks(count);
        get_memory_leaks(leaks.data());

        for (int i = 0; i < count; i++)
            sLog.warn("heap meory leakage in usage {} ({} bytes)", get_memory_usage_cstr(leaks[i].usage), leaks[i].current);
    }

    return 0;
}