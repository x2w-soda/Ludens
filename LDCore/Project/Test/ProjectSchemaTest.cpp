#include <Extra/doctest/doctest.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/Memory.h>
#include <cstring>

namespace fs = std::filesystem;
using namespace LD;

TEST_CASE("ProjectSchema")
{
    const char schemaTOML[] = R"(
[ludens_project]
version_major = 0
version_minor = 0
version_patch = 0
name = "hello world"
assets = "assets.toml"
scenes = [
    "scenes/scene1.toml",
    "./scenes/scene2.toml",
    "./scenes/scene3.toml",
]
)";
    std::string err;
    TOMLDocument doc = TOMLDocument::create();
    bool ok = doc.parse(schemaTOML, strlen(schemaTOML), err);
    CHECK(ok);

    Project proj = Project::create(fs::path("./directory"));
    ProjectSchema::load_project(proj, doc);
    TOMLDocument::destroy(doc);

    CHECK(proj.get_name() == "hello world");

    fs::path assetsPath = proj.get_assets_path();
    CHECK(assetsPath == fs::path("directory/assets.toml"));

    std::vector<fs::path> scenePaths;
    proj.get_scene_paths(scenePaths);
    CHECK(scenePaths.size() == 3);
    CHECK(scenePaths[0] == fs::path("directory/scenes/scene1.toml"));
    CHECK(scenePaths[1] == fs::path("directory/scenes/scene2.toml"));
    CHECK(scenePaths[2] == fs::path("directory/scenes/scene3.toml"));

    Project::destroy(proj);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}