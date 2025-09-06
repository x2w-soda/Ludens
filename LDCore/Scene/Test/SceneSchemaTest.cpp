#include <Extra/doctest/doctest.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/Memory.h>

using namespace LD;

TEST_CASE("SceneSchema")
{
    const char toml[] = R"(
[ludens_scene]
version = 0

[[component]]
script = 6
name = "model"
type = "Mesh"
transform = { position = [0.0, 0.0, 0.0], rotation = [0.0, 0.0, 0.0], scale = [0.01, 0.01, 0.01] }
auid = 1
cuid = 345
)";
    std::string error;
    TOMLDocument doc = TOMLDocument::create();
    bool ok = doc.parse(toml, strlen(toml), error);
    CHECK(ok);

    Scene scene = Scene::create();
    SceneSchema::load_scene(scene, doc);
    CHECK(scene);

    std::vector<CUID> roots;
    scene.get_root_components(roots);
    CHECK(roots.size() == 1);

    {
        Transform transform;
        ok = scene.get_component_transform(roots[0], transform);
        CHECK(ok);
        CHECK(transform.position == Vec3(0.0f, 0.0f, 0.0f));
        CHECK(transform.rotation == Vec3(0.0f, 0.0f, 0.0f));
        CHECK(transform.scale == Vec3(0.01f, 0.01f, 0.01f));

        ComponentBase* base = scene.get_component_base(roots[0]);
        std::string name(base->name);
        CHECK(name == "model");
        CHECK(base->type == COMPONENT_TYPE_MESH);
        CHECK(base->id == 345);
    }

    Scene::destroy(scene);
    TOMLDocument::destroy(doc);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}