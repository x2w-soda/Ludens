#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/URI.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>

using namespace LD;

TEST_CASE("DocumentURI normalized")
{
    URI uri("ld://Doc/Manual/");
    document_uri_normalize(uri);
    CHECK(uri.string() == "ld://Doc/Manual/index.md");

    uri = URI("ld://Doc/Manual");
    document_uri_normalize(uri);
    CHECK(uri.string() == "ld://Doc/Manual/index.md");

    uri = URI("ld://Doc/Manual/Hello.md");
    document_uri_normalize(uri);
    CHECK(uri.string() == "ld://Doc/Manual/Hello.md");
}

TEST_CASE("DocumentURI md path")
{
    FS::Path path = document_uri_md_path("ld://Doc/Manual");
    CHECK(path == "Manual/index.md");

    path = document_uri_md_path("ld://Doc/Manual");
    CHECK(path == "Manual/index.md");

    path = document_uri_md_path("ld://Doc/Manual/index.md");
    CHECK(path == "Manual/index.md");
}