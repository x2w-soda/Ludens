#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct DocumentBuilderInfo
{
    FS::Path doxygenXMLPath; /// path to Doxygen index.xml file
};

struct DocumentBuilder : Handle<struct DocumentBuilderObj>
{
    static DocumentBuilder create(const DocumentBuilderInfo& builderI);
    static void destroy(DocumentBuilder builderI);
};

} // namespace LD