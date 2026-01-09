#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <cstring>

using namespace LD;

TEST_CASE("ProjectSchema")
{
    const char schemaTOML[] = R"(
[ludensProject]
versionMajor = 0
versionMinor = 0
versionPatch = 0
name = "hello world"
assets = "assets.toml"
scenes = [
    "scenes/scene1.toml",
    "./scenes/scene2.toml",
    "./scenes/scene3.toml",
]

[settings.startup]
windowWidth = 1234
windowHeight = 5678
windowName = 'Foo'
)";
    Project proj = Project::create(FS::Path("./directory"));
    std::string err;
    bool ok = ProjectSchema::load_project_from_source(proj, View(schemaTOML, sizeof(schemaTOML) - 1), err);
    CHECK(ok);

    CHECK(proj.get_name() == "hello world");

    FS::Path assetsPath = proj.get_assets_path();
    CHECK(assetsPath == FS::Path("directory/assets.toml"));

    std::vector<FS::Path> scenePaths;
    proj.get_scene_paths(scenePaths);
    CHECK(scenePaths.size() == 3);
    CHECK(scenePaths[0] == FS::Path("directory/scenes/scene1.toml"));
    CHECK(scenePaths[1] == FS::Path("directory/scenes/scene2.toml"));
    CHECK(scenePaths[2] == FS::Path("directory/scenes/scene3.toml"));

    ProjectStartupSettings startupS = proj.get_settings().get_startup_settings();
    CHECK(startupS.get_window_width() == 1234);
    CHECK(startupS.get_window_height() == 5678);
    CHECK(startupS.get_window_name() == "Foo");

    Project::destroy(proj);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}