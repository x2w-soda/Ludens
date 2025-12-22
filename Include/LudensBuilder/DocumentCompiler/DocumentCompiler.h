#pragma once

#include <Ludens/Header/Handle.h>
#include <filesystem>

namespace LD {

struct DocumentCompilerInfo
{
    std::filesystem::path pathToDoxygenXML; /// path to Doxygen index.xml file
};

struct DocumentCompiler : Handle<struct DocumentCompilerObj>
{
    static DocumentCompiler create(const DocumentCompilerInfo& compilerInfo);
    static void destroy(DocumentCompiler compiler);
};

} // namespace LD