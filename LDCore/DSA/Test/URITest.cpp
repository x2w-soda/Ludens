#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/URI.h>

using namespace LD;

TEST_CASE("URI Scheme")
{
    URI uri("https://");
    CHECK(uri.scheme() == "https");

    uri = URI("AsseT://");
    View v = uri.scheme();
    CHECK(v == "AsseT");

    uri = URI("://");
    CHECK_FALSE(uri.scheme());
}

TEST_CASE("URI Path")
{
    URI uri("doc:///root/file");
    CHECK(uri.scheme() == "doc");
    CHECK_FALSE(uri.authority());
    CHECK(uri.path() == "root/file");

    uri = URI("doc://manual/a/b/c/d/e.md");
    CHECK(uri.scheme() == "doc");
    CHECK(uri.authority() == "manual");
    CHECK(uri.path() == "a/b/c/d/e.md");
}

TEST_CASE("URI Query Fragment")
{
    URI uri("doc://media/icon.png?color=red#top");
    CHECK(uri.scheme() == "doc");
    CHECK(uri.authority() == "media");
    CHECK(uri.path() == "icon.png");
    CHECK(uri.query() == "color=red");
    CHECK(uri.fragment() == "top");
}

TEST_CASE("URI Malformed")
{
    URI uri("");
    CHECK_FALSE(uri.scheme());
    CHECK_FALSE(uri.authority());
    CHECK_FALSE(uri.path());
    CHECK_FALSE(uri.query());
    CHECK_FALSE(uri.fragment());

    uri = URI("plain_text_no_scheme");
    CHECK_FALSE(uri.scheme());
    CHECK(uri.authority() == "plain_text_no_scheme");
    CHECK_FALSE(uri.path());
    CHECK_FALSE(uri.query());
    CHECK_FALSE(uri.fragment());

    // handle the case where query comes after fragment
    uri = URI("doc://media/icon.png#top?color=red");
    CHECK(uri.scheme() == "doc");
    CHECK(uri.authority() == "media");
    CHECK(uri.path() == "icon.png");
    CHECK(uri.query() == "color=red");
    CHECK(uri.fragment() == "top");
}

TEST_CASE("URI HTTP")
{
    URI uri("https://github.com/x2w-soda/Ludens.git");
    CHECK(uri.scheme() == "https");
    CHECK(uri.authority() == "github.com");
    CHECK(uri.path() == "x2w-soda/Ludens.git");
    CHECK_FALSE(uri.query());
    CHECK_FALSE(uri.fragment());

    uri = URI("https://en.wikipedia.org/wiki/Uniform_Resource_Identifier#Syntax");
    CHECK(uri.scheme() == "https");
    CHECK(uri.authority() == "en.wikipedia.org");
    CHECK(uri.path() == "wiki/Uniform_Resource_Identifier");
    CHECK_FALSE(uri.query());
    CHECK(uri.fragment() == "Syntax");

    uri = URI("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    CHECK(uri.scheme() == "https");
    CHECK(uri.authority() == "www.youtube.com");
    CHECK(uri.path() == "watch");
    CHECK(uri.query() == "v=dQw4w9WgXcQ");
    CHECK_FALSE(uri.fragment());
}