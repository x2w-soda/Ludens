#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneSchema.h>

#include <cstdint>
#include <format>

#include "SceneSchemaKeys.h"

namespace LD {

/// @brief Saves Scene to TOML schema.
class SceneSchemaSaver
{
public:
    SceneSchemaSaver() = default;
    SceneSchemaSaver(const SceneSchemaSaver&) = delete;
    ~SceneSchemaSaver();

    SceneSchemaSaver& operator=(const SceneSchemaSaver&) = delete;

    bool save_scene(Scene scene, std::string& toml, std::string& err);

    static bool save_audio_source_component(SceneSchemaSaver& saver, Scene::Component comp);
    static bool save_camera_component(SceneSchemaSaver& saver, Scene::Component comp);
    static bool save_mesh_component(SceneSchemaSaver& saver, Scene::Component comp);
    static bool save_sprite_2d_component(SceneSchemaSaver& saver, Scene::Component comp);

private:
    static void save_component(SceneSchemaSaver& saver, Scene::Component comp);

private:
    Scene mScene{};
    HashMap<SUID, Vector<SUID>> mChildMap;
    TOMLWriter mWriter{};
};

/// @brief Loads Scene from TOML schema.
class SceneSchemaLoader
{
public:
    SceneSchemaLoader() = default;
    SceneSchemaLoader(const SceneSchemaLoader&) = delete;
    ~SceneSchemaLoader();

    SceneSchemaLoader& operator=(const SceneSchemaLoader&) = delete;

    bool load_scene(Scene scene, const View& toml, std::string& err);

    static Scene::Component load_audio_source_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName);
    static Scene::Component load_camera_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName);
    static Scene::Component load_mesh_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName);
    static Scene::Component load_sprite_2d_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName);

private:
    static Scene::Component load_component(SceneSchemaLoader& loader, TOMLValue compTOML);

private:
    Scene mScene{};
    TOMLDocument mDoc{};
};

static bool load_rect(Rect& rect, TOMLValue rectTOML);
static bool load_transform(TransformEx& transform, TOMLValue transformTOML);
static bool load_transform_2d(Transform2D& transform, TOMLValue transformTOML);

static void save_transform(const TransformEx& transform, TOMLWriter writer, const char* key);
static void save_transform_2d(const Transform2D& transform, TOMLWriter writer);

// clang-format off
struct
{
    ComponentType type;
    const char* compTypeName;
    Scene::Component (*load)(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName);
    bool (*save)(SceneSchemaSaver& saver, Scene::Component comp);
} sSceneSchemaTable[] = {
    {COMPONENT_TYPE_DATA,           "Data",        nullptr,                                           nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE,   "AudioSource", &SceneSchemaLoader::load_audio_source_component,   &SceneSchemaSaver::save_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,      "Transform",   nullptr,                                           nullptr},
    {COMPONENT_TYPE_CAMERA,         "Camera",      &SceneSchemaLoader::load_camera_component,         &SceneSchemaSaver::save_camera_component},
    {COMPONENT_TYPE_MESH,           "Mesh",        &SceneSchemaLoader::load_mesh_component,           &SceneSchemaSaver::save_mesh_component},
    {COMPONENT_TYPE_SPRITE_2D,      "Sprite2D",    &SceneSchemaLoader::load_sprite_2d_component,      &SceneSchemaSaver::save_sprite_2d_component},
};
// clang-format on

static_assert(sizeof(sSceneSchemaTable) / sizeof(*sSceneSchemaTable) == COMPONENT_TYPE_ENUM_COUNT);

Scene::Component SceneSchemaLoader::load_component(SceneSchemaLoader& loader, TOMLValue compTOML)
{
    if (!loader.mScene || !compTOML || !compTOML.is_table())
        return {};

    std::string type;
    TOMLValue typeTOML = compTOML["type"];
    if (!typeTOML || !typeTOML.get_string(type))
        return {};

    std::string name;
    TOMLValue nameTOML = compTOML["name"];
    if (!nameTOML || !nameTOML.get_string(name))
        return {};

    SUID compSUID;
    TOMLValue compIDTOML = compTOML["cuid"];
    if (!compIDTOML || !compIDTOML.get_u32(compSUID))
        return {};

    Scene::Component comp{};

    for (int i = 1; i < (int)COMPONENT_TYPE_ENUM_COUNT; i++)
    {
        if (type == sSceneSchemaTable[i].compTypeName)
        {
            LD_ASSERT(sSceneSchemaTable[i].load);
            comp = sSceneSchemaTable[i].load(loader, compTOML, compSUID, name.c_str());
            LD_ASSERT(comp); // TODO: deserialization error handling.
        }
    }

    AssetID scriptID = 0;
    TOMLValue scriptIDTOML = compTOML["script"];
    if (scriptIDTOML)
        scriptIDTOML.get_u32(scriptID);

    return comp;
}

Scene::Component SceneSchemaLoader::load_audio_source_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;

    Scene::AudioSource source(scene.create_component_serial(COMPONENT_TYPE_AUDIO_SOURCE, compName, (SUID)0, compSUID));
    if (!source)
        return {};

    AssetID clipID = 0;
    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.get_u32(clipID);

    source.set_clip_asset(clipID);

    float pan = 0.5f;
    float volumeLinear = 1.0f;

    TOMLValue panTOML = compTOML["pan"];
    if (panTOML)
        panTOML.get_f32(pan);

    source.set_pan(pan);

    TOMLValue volumeTOML = compTOML["volume_linear"];
    if (volumeTOML)
        volumeTOML.get_f32(volumeLinear);

    source.set_volume_linear(volumeLinear);

    return {};
}

Scene::Component SceneSchemaLoader::load_camera_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;

    Scene::Camera camera(scene.create_component_serial(COMPONENT_TYPE_CAMERA, compName, (SUID)0, compSUID));
    if (!camera)
        return {};

    TOMLValue floatTOML{};
    TOMLValue toml = compTOML["transform"];
    TransformEx transform{};
    if (!load_transform(transform, toml))
        return {};

    camera.mark_transform_dirty();

    bool isPerspective = false;
    bool isMainCamera = false;

    toml = compTOML["isPerspective"];
    if (!toml || !toml.get_bool(isPerspective))
        return {};

    toml = compTOML["isMainCamera"];
    if (!toml || !toml.get_bool(isMainCamera))
        return {};

    if (isPerspective)
    {
        toml = compTOML["perspective"];
        if (!toml || !toml.is_table())
            return {}; // missing perspective info

        float fovDegrees;
        floatTOML = toml["fov"];
        if (!floatTOML || !floatTOML.get_f32(fovDegrees))
            return {};

        CameraPerspectiveInfo perspective{};
        perspective.fov = LD_TO_RADIANS(fovDegrees);

        floatTOML = toml["nearClip"];
        if (!floatTOML || !floatTOML.get_f32(perspective.nearClip))
            return {};

        floatTOML = toml["farClip"];
        if (!floatTOML || !floatTOML.get_f32(perspective.farClip))
            return {};

        // overridden later when screen extent is known.
        perspective.aspectRatio = 1.0f;

        camera.set_perspective(perspective);
    }
    else
    {
        toml = compTOML["orthographic"];
        if (!toml || !toml.is_table())
            return {}; // missing orthographic info

        CameraOrthographicInfo ortho{};

        floatTOML = toml["left"];
        if (!floatTOML || !floatTOML.get_f32(ortho.left))
            return {};

        floatTOML = toml["right"];
        if (!floatTOML || !floatTOML.get_f32(ortho.right))
            return {};

        floatTOML = toml["bottom"];
        if (!floatTOML || !floatTOML.get_f32(ortho.bottom))
            return {};

        floatTOML = toml["top"];
        if (!floatTOML || !floatTOML.get_f32(ortho.top))
            return {};

        floatTOML = toml["nearClip"];
        if (!floatTOML || !floatTOML.get_f32(ortho.nearClip))
            return {};

        floatTOML = toml["farClip"];
        if (!floatTOML || !floatTOML.get_f32(ortho.farClip))
            return {};

        camera.set_orthographic(ortho);
    }

    return camera;
}

Scene::Component SceneSchemaLoader::load_mesh_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;

    Scene::Component comp = scene.create_component_serial(COMPONENT_TYPE_MESH, compName, (SUID)0, compSUID);
    if (!comp)
        return {};

    Scene::Mesh mesh(comp);

    TransformEx transform;
    TOMLValue transformTOML = compTOML["transform"];
    if (!load_transform(transform, transformTOML))
        return {};

    mesh.set_transform(transform);
    mesh.mark_transform_dirty();

    TOMLValue auidTOML = compTOML["auid"];
    AssetID assetID = 0;
    auidTOML.get_u32(assetID);
    mesh.set_mesh_asset(assetID);

    return Scene::Component(mesh.data());
}

Scene::Component SceneSchemaLoader::load_sprite_2d_component(SceneSchemaLoader& loader, TOMLValue compTOML, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;

    Scene::Sprite2D sprite(scene.create_component_serial(COMPONENT_TYPE_SPRITE_2D, compName, (SUID)0, compSUID));
    if (!sprite)
        return {};

    Rect rect;
    TOMLValue localTOML = compTOML.get_key("local", TOML_TYPE_TABLE);
    if (!load_rect(rect, localTOML))
        return {};

    sprite.set_rect(rect);

    Transform2D transform;
    TOMLValue transformTOML = compTOML["transform"];
    if (!load_transform_2d(transform, transformTOML))
        return {};

    sprite.get_transform_2d(transform);
    sprite.mark_transform_dirty();

    AssetID assetID = 0;
    TOMLValue auidTOML = compTOML["auid"];
    if (auidTOML)
        auidTOML.get_u32(assetID);

    sprite.set_texture_2d_asset(assetID);

    uint32_t zDepth = 0;
    TOMLValue zDepthTOML = compTOML["zDepth"];
    if (zDepthTOML)
        zDepthTOML.get_u32(zDepth);

    sprite.set_z_depth(zDepth);

    return sprite;
}

static bool load_rect(Rect& rect, TOMLValue rectTOML)
{
    LD_ASSERT(rectTOML && rectTOML.is_table());

    return TOMLUtil::load_rect_table(rect, rectTOML);
}

static bool load_transform(TransformEx& transform, TOMLValue transformTOML)
{
    if (!transformTOML || !transformTOML.is_table())
        return false;

    TOMLValue positionTOML = transformTOML["position"];
    if (!positionTOML || !positionTOML.is_array() || positionTOML.size() != 3)
        return false;

    positionTOML[0].get_f32(transform.position.x);
    positionTOML[1].get_f32(transform.position.y);
    positionTOML[2].get_f32(transform.position.z);

    TOMLValue rotationTOML = transformTOML["rotation"];
    if (!rotationTOML || !rotationTOML.is_array() || rotationTOML.size() != 3)
        return false;

    rotationTOML[0].get_f32(transform.rotation.x);
    rotationTOML[1].get_f32(transform.rotation.y);
    rotationTOML[2].get_f32(transform.rotation.z);
    transform.rotation = Quat::from_euler(transform.rotationEuler);

    TOMLValue scaleTOML = transformTOML["scale"];
    if (!scaleTOML || !scaleTOML.is_array() || scaleTOML.size() != 3)
        return false;

    scaleTOML[0].get_f32(transform.scale.x);
    scaleTOML[1].get_f32(transform.scale.y);
    scaleTOML[2].get_f32(transform.scale.z);

    return true;
}

static bool load_transform_2d(Transform2D& transform, TOMLValue transformTOML)
{
    if (!transformTOML || !transformTOML.is_table())
        return false;

    TOMLValue positionTOML = transformTOML["position"];
    if (!positionTOML || !positionTOML.is_array() || positionTOML.size() != 2)
        return false;

    positionTOML[0].get_f32(transform.position.x);
    positionTOML[1].get_f32(transform.position.y);

    TOMLValue rotationTOML = transformTOML["rotation"];
    if (!rotationTOML || !rotationTOML.is_float())
        return false;

    rotationTOML.get_f32(transform.rotation);

    TOMLValue scaleTOML = transformTOML["scale"];
    if (!scaleTOML || !scaleTOML.is_array() || scaleTOML.size() != 2)
        return false;

    scaleTOML[0].get_f32(transform.scale.x);
    scaleTOML[1].get_f32(transform.scale.y);

    return true;
}

SceneSchemaSaver::~SceneSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool SceneSchemaSaver::save_scene(Scene scene, std::string& toml, std::string& err)
{
    mScene = scene;

    mWriter = TOMLWriter::create();

    mWriter.begin();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MAJOR).value_i32(LD_VERSION_MAJOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MINOR).value_i32(LD_VERSION_MINOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_PATCH).value_i32(LD_VERSION_PATCH);
    mWriter.end_table();

    mWriter.begin_array_table("component");

    Vector<Scene::Component> roots;
    mScene.get_root_components(roots);
    for (Scene::Component root : roots)
    {
        save_component(*this, root);
    }

    mWriter.end_array_table();

    mWriter.begin_table("hierarchy");
    for (auto ite : mChildMap)
    {
        std::string parentID = std::to_string(ite.first);
        mWriter.key(parentID.c_str()).begin_array();

        for (CUID childrenID : ite.second)
            mWriter.value_u32(childrenID);

        mWriter.end_array();
    }
    mWriter.end_table();
    mWriter.end(toml);

    TOMLWriter::destroy(mWriter);
    mWriter = {};

    return true;
}

bool SceneSchemaSaver::save_audio_source_component(SceneSchemaSaver& saver, Scene::Component comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    Scene::AudioSource source(comp);
    if (!source)
        return false;

    TOMLWriter writer = saver.mWriter;
    writer.key("auid").value_u32(source.get_clip_asset());
    writer.key("pan").value_f32(source.get_pan());
    writer.key("volumeLinear").value_f32(source.get_volume_linear());

    return true;
}

bool SceneSchemaSaver::save_camera_component(SceneSchemaSaver& saver, Scene::Component comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    Scene::Camera camera(comp);
    if (!camera)
        return false;

    bool ok;
    TOMLWriter writer = saver.mWriter;
    TransformEx transform;
    camera.get_transform(transform);
    save_transform(transform, writer, "transform");

    writer.key("isPerspective").value_bool(camera.is_perspective());
    writer.key("isMainCamera").value_bool(camera.is_main_camera());

    if (camera.is_perspective())
    {
        writer.begin_inline_table("perspective");

        CameraPerspectiveInfo perspective;
        ok = camera.get_perspective_info(perspective);
        LD_ASSERT(ok);

        float fovDegrees = (float)LD_TO_DEGREES(perspective.fov);
        writer.key("fov").value_f32(fovDegrees);
        writer.key("nearClip").value_f32(perspective.nearClip);
        writer.key("farClip").value_f32(perspective.farClip);

        writer.end_inline_table();
    }
    else
    {
        writer.begin_inline_table("orthographic");

        CameraOrthographicInfo ortho;
        ok = camera.get_orthographic_info(ortho);
        LD_ASSERT(ok);

        writer.key("left").value_f32(ortho.left);
        writer.key("right").value_f32(ortho.right);
        writer.key("bottom").value_f32(ortho.bottom);
        writer.key("top").value_f32(ortho.top);
        writer.key("nearClip").value_f32(ortho.nearClip);
        writer.key("farClip").value_f32(ortho.farClip);

        writer.end_inline_table();
    }

    return true;
}

bool SceneSchemaSaver::save_mesh_component(SceneSchemaSaver& saver, Scene::Component comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    Scene::Mesh mesh(comp);
    if (!comp)
        return false;

    TOMLWriter writer = saver.mWriter;
    TransformEx transform;
    mesh.get_transform(transform);
    save_transform(transform, writer, "transform");
    writer.key("auid").value_u32(mesh.get_mesh_asset());

    return true;
}

bool SceneSchemaSaver::save_sprite_2d_component(SceneSchemaSaver& saver, Scene::Component comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    Scene::Sprite2D sprite(comp);
    if (!sprite)
        return false;

    Rect rect = sprite.get_rect();
    TOMLWriter writer = saver.mWriter;
    writer.begin_inline_table("local");
    TOMLUtil::save_rect_table(rect, writer);
    writer.end_inline_table();

    Transform2D transform;
    if (!sprite.get_transform_2d(transform))
        return false;
    save_transform_2d(transform, writer);

    writer.key("auid").value_u32(sprite.get_texture_2d_asset());
    writer.key("zDepth").value_u32(sprite.get_z_depth());

    return true;
}

void SceneSchemaSaver::save_component(SceneSchemaSaver& saver, Scene::Component root)
{
    LD_ASSERT(root);

    TOMLWriter writer = saver.mWriter;

    writer.begin_table();

    ComponentType type = root.type();
    std::string compTypeName(sSceneSchemaTable[(int)type].compTypeName);
    writer.key("type").value_string(compTypeName);
    writer.key("name").value_string(root.get_name());
    writer.key("cuid").value_u32(root.suid());
    writer.key("script").value_u32(root.get_script_asset_id());

    LD_ASSERT(sSceneSchemaTable[(int)type].save);
    bool ok = sSceneSchemaTable[(int)type].save(saver, root);
    LD_ASSERT(ok); // TODO: error handling path

    writer.end_table();

    // recursively save entire subtree
    Vector<Scene::Component> children;
    root.get_children(children);

    for (Scene::Component child : children)
    {
        LD_ASSERT(child);
        saver.mChildMap[root.suid()].push_back(child.suid());
        save_component(saver, child);
    }
}

static void save_transform(const TransformEx& transform, TOMLWriter writer, const char* key)
{
    LD_ASSERT(writer);

    writer.key(key).begin_inline_table();

    writer.key("position").begin_array();
    writer.value_f32(transform.position.x);
    writer.value_f32(transform.position.y);
    writer.value_f32(transform.position.z);
    writer.end_array();

    writer.key("rotation").begin_array();
    writer.value_f32(transform.rotationEuler.x);
    writer.value_f32(transform.rotationEuler.y);
    writer.value_f32(transform.rotationEuler.z);
    writer.end_array();

    writer.key("scale").begin_array();
    writer.value_f32(transform.scale.x);
    writer.value_f32(transform.scale.y);
    writer.value_f32(transform.scale.z);
    writer.end_array();

    writer.end_inline_table();
}

static void save_transform_2d(const Transform2D& transform, TOMLWriter writer)
{
    LD_ASSERT(writer);

    writer.key("transform").begin_inline_table();

    writer.key("position").begin_array();
    writer.value_f32(transform.position.x);
    writer.value_f32(transform.position.y);
    writer.end_array();

    writer.key("rotation").value_f32(transform.rotation);

    writer.key("scale").begin_array();
    writer.value_f32(transform.scale.x);
    writer.value_f32(transform.scale.y);
    writer.end_array();

    writer.end_inline_table();
}

SceneSchemaLoader::~SceneSchemaLoader()
{
    if (mDoc)
        TOMLDocument::destroy(mDoc);
}

bool SceneSchemaLoader::load_scene(Scene scene, const View& toml, std::string& err)
{
    mDoc = TOMLDocument::create();
    mScene = scene;

    if (!TOMLParser::parse(mDoc, toml, err))
        return false;

    scene.reset();

    TOMLValue sceneTOML = mDoc.get(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    if (!sceneTOML || sceneTOML.type() != TOML_TYPE_TABLE)
        return false;

    int32_t version;
    TOMLValue versionTOML = sceneTOML[SCENE_SCHEMA_KEY_VERSION_MAJOR];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MAJOR)
        return false;

    versionTOML = sceneTOML[SCENE_SCHEMA_KEY_VERSION_MINOR];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MINOR)
        return false;

    versionTOML = sceneTOML[SCENE_SCHEMA_KEY_VERSION_PATCH];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_PATCH)
        return false;

    // extract component tables
    HashMap<SUID, TOMLValue> compValues;
    TOMLValue componentsTOML = mDoc.get("component");
    if (componentsTOML && componentsTOML.is_array())
    {
        int count = componentsTOML.size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue compTOML = componentsTOML[i];
            if (!compTOML.is_table())
                continue;

            SUID compSUID;
            TOMLValue compIDTOML = compTOML["cuid"];
            if (compIDTOML && compIDTOML.get_u32(compSUID))
                compValues[compSUID] = compTOML;
        }
    }

    for (auto it : compValues)
    {
        TOMLValue compTOML = it.second;
        CUID compCUID = load_component(*this, compTOML);
        LD_ASSERT(compCUID); // TODO: error handling
    }

    TOMLValue hierarchyTOML = mDoc.get("hierarchy");
    if (hierarchyTOML && hierarchyTOML.is_table())
    {
        Vector<std::string> keys;
        hierarchyTOML.get_keys(keys);
        for (const std::string& key : keys)
        {
            SUID parent = static_cast<SUID>(std::stoul(key));
            TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
            if (!childrenTOML || !childrenTOML.is_array())
                continue;

            int count = childrenTOML.size();
            for (int i = 0; i < count; i++)
            {
                SUID child;
                if (!childrenTOML[i].get_u32(child))
                    continue;

                // TODO: reparent takes cuid
                LD_UNREACHABLE;
                scene.reparent(child, parent);
            }
        }
    }

    TOMLDocument::destroy(mDoc);
    mDoc = {};

    return true;
}

//
// Public API
//

bool SceneSchema::load_scene_from_source(Scene scene, const View& toml, std::string& err)
{
    LD_PROFILE_SCOPE;

    SceneSchemaLoader loader;
    if (!loader.load_scene(scene, toml, err))
        return false;

    return true;
}

bool SceneSchema::load_scene_from_file(Scene scene, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    View tomlView((const char*)toml.data(), toml.size());
    return load_scene_from_source(scene, tomlView, err);
}

bool SceneSchema::save_scene(Scene scene, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    std::string toml;
    SceneSchemaSaver saver;
    if (!saver.save_scene(scene, toml, err))
        return false;

    View tomlView(toml.data(), toml.size());
    return FS::write_file_and_swap_backup(savePath, tomlView, err);
}

} // namespace LD
