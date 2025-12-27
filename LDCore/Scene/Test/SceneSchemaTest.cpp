#include <Extra/doctest/doctest.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/Memory.h>

using namespace LD;

TEST_CASE("SceneSchema")
{
    const char toml[] = R"(
[ludens_scene]
version_major = 0
version_minor = 0
version_patch = 0

[[component]]
script = 6
name = "model"
type = "Mesh"
transform = { position = [0.0, 0.0, 0.0], rotation = [0.0, 0.0, 0.0], scale = [0.01, 0.01, 0.01] }
auid = 1
cuid = 345

[[component]]
name = "sprite1"
type = "Sprite2D"
local = { x = 1, y = 1, w = 10, h = 10 }
transform = { position = [3.0, 4.0], rotation = 345.0, scale = [2.0, 3.0] }
auid = 2
cuid = 300
)";

    Scene scene = Scene::create({});
    CHECK(scene);
    SceneSchema::load_scene_from_source(scene, toml, strlen(toml));

    std::vector<CUID> roots;
    scene.get_root_components(roots);
    CHECK(roots.size() == 2);

    {
        CUID meshCID = 345;
        TransformEx transform;
        bool ok = scene.get_component_transform(meshCID, transform);
        CHECK(ok);
        CHECK(transform.position == Vec3(0.0f, 0.0f, 0.0f));
        CHECK(transform.rotationEuler == Vec3(0.0f, 0.0f, 0.0f));
        CHECK(transform.scale == Vec3(0.01f, 0.01f, 0.01f));

        ComponentBase* base = scene.get_component_base(roots[0]);
        std::string name(base->name);
        CHECK(name == "model");
        CHECK(base->type == COMPONENT_TYPE_MESH);
        CHECK(base->id == 345);
    }

    {
        CUID spriteCID = 300;
        Transform2D transform;
        bool ok = scene.get_component_transform2d(spriteCID, transform);
        CHECK(ok);
        CHECK(transform.position == Vec2(3.0f, 4.0f));
        CHECK(transform.rotation == 345.0f);
        CHECK(transform.scale == Vec2(2.0f, 3.0f));
    }

    Scene::destroy(scene);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}
