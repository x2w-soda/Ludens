#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include "DocumentBuilderTest.h"

#include <Ludens/Memory/Memory.h>
#include <Ludens/System/FileSystem.h>

#include <algorithm>
#include <format>
#include <iostream>

using namespace LD;

Document require_document(const char* md, const char* uri)
{
    std::string err;
    DocumentInfo info{};
    info.md = View(md, strlen(md));
    info.uri = uri;
    Document doc = Document::create(info, err);
    REQUIRE(doc);

    return doc;
}

bool require_documents(DocumentRegistry reg, HashSet<Vector<byte>*>& storage, const std::filesystem::path& docPath)
{
    std::string err;

    Vector<FS::Path> mdPaths;
    REQUIRE(FS::get_directory_content(docPath, mdPaths, true, err));

    for (const FS::Path& mdPath : mdPaths)
    {
        if (FS::is_directory(mdPath) || mdPath.extension() != ".md")
            continue;

        auto* mdStorage = heap_new<Vector<byte>>(MEMORY_USAGE_DOCUMENT);
        REQUIRE(FS::read_file_to_vector(mdPath, *mdStorage, err));
        storage.insert(mdStorage);

        std::string uriString = std::filesystem::relative(mdPath, docPath).string();
        std::replace(uriString.begin(), uriString.end(), '\\', '/');
        uriString = "ld://Doc/" + uriString;

        DocumentInfo info{};
        info.md = View((const char*)mdStorage->data(), mdStorage->size());
        info.uri = uriString.c_str();
        REQUIRE(reg.add_document(info, err));

        std::cout << std::format("DocumentBuilderTest: loaded {}", uriString) << std::endl;
    }

    return true;
}

void release_document_storage(HashSet<Vector<byte>*>& storage)
{
    for (auto it : storage)
        heap_delete<Vector<byte>>(it);

    storage.clear();
}