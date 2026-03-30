#include <Ludens/CommandLine/ArgParser.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/System/FileSystem.h>

#include "EditorApplication.h"

#include <iostream>

#define ARGV_PROJECT_SCHEMA_PATH 0

namespace LD {

class EditorArgs
{
public:
    EditorArgs() = delete;
    EditorArgs(int argc, char** argv);
    EditorArgs(const EditorArgs&) = delete;
    ~EditorArgs();

    inline bool get_project_schema_path(FS::Path& outPath) const
    {
        if (mProjectSchemaPath.has_value())
        {
            outPath = FS::absolute(mProjectSchemaPath.value());
            return true;
        }

        outPath.clear();
        return false;
    }

    EditorArgs& operator=(const EditorArgs&) = delete;

private:
    ArgParser mParser;
    Optional<FS::Path> mProjectSchemaPath;
};

EditorArgs::EditorArgs(int argc, char** argv)
{
    Array<ArgOption, 1> options;
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

EditorArgs::~EditorArgs()
{
    ArgParser::destroy(mParser);
}

} // namespace LD

int main(int argc, char** argv)
{
    {
        LD::FS::Path projectSchemaPath;
        LD::EditorArgs args(argc, argv);
        LD::EditorApplicationInfo appInfo{};

        if (args.get_project_schema_path(projectSchemaPath))
            appInfo.projectSchemaPath = &projectSchemaPath;

        LD::EditorApplication editorApp(appInfo);
        editorApp.run();
    }

    int count = LD::get_memory_leaks(nullptr);

    if (count > 0)
    {
        LD::Vector<LD::MemoryProfile> leaks(count);
        LD::get_memory_leaks(leaks.data());

        for (int i = 0; i < count; i++)
            std::cout << "memory leak in usage " << LD::get_memory_usage_cstr(leaks[i].usage) << ": " << leaks[i].current << " bytes" << std::endl;
    }
}