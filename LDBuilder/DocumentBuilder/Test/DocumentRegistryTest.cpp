#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/System/FileSystem.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>
#include <LudensUtil/LudensLFS.h>

#include "DocumentBuilderTest.h"

using namespace LD;

// Offline total-validation of engine documents.
TEST_CASE("DocumentRegistry Validation" * doctest::skip(!LudensLFS::get_root_directory_path()))
{
    FS::Path rootDirPath;
    REQUIRE(LudensLFS::get_root_directory_path(&rootDirPath));

    FS::Path docsDirPath = FS::absolute(rootDirPath / "Docs");
    REQUIRE(FS::exists(docsDirPath));

    HashSet<Vector<byte>*> docStorage;
    DocumentRegistry reg = DocumentRegistry::create();
    REQUIRE(require_documents(reg, docStorage, docsDirPath));

    DocumentRegistryValidator validator{};
    validator.onMiscURI = [](View) { return false; };
    CHECK(reg.validate(validator));

    DocumentRegistry::destroy(reg);
    release_document_storage(docStorage);

    CHECK_FALSE(get_memory_leaks(nullptr));
}