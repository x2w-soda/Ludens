#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/URI.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>

using namespace LD;

TEST_CASE("DocumentURI normalized path")
{
    URI uri("ld://Doc/Manual/");
    std::string str;
    
    str = document_uri_normalized_path(uri);
    CHECK(str == "Manual");

    str = document_uri_normalized_path(URI("ld://Doc/Manual"));
    CHECK(str == "Manual");

    str = document_uri_normalized_path(URI("ld://Doc/Manual/Hello.md"));
    CHECK(str == "Manual/Hello");
}

TEST_CASE("DocumentURI md path")
{
    FS::Path path = document_md_path_from_uri_path("Manual");
    CHECK(path == "Manual/index.md");

    path = document_md_path_from_uri_path("Manual/index.md");
    CHECK(path == "Manual/index.md");
}