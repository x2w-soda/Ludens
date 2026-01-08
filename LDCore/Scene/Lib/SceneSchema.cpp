#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/Memory.h>

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

    static bool save_audio_source_component(SceneSchemaSaver& saver, CUID compID, const char* compName);
    static bool save_camera_component(SceneSchemaSaver& saver, CUID compID, const char* compName);
    static bool save_mesh_component(SceneSchemaSaver& saver, CUID compID, const char* compName);
    static bool save_sprite_2d_component(SceneSchemaSaver& saver, CUID compID, const char* compName);

private:
    static void save_component(SceneSchemaSaver& saver, ComponentBase* rootC);

private:
    Scene mScene{};
    HashMap<CUID, Vector<CUID>> mChildMap;
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

    static bool load_audio_source_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName);
    static bool load_camera_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName);
    static bool load_mesh_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName);
    static bool load_sprite_2d_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName);

private:
    static CUID load_component(SceneSchemaLoader& loader, TOMLValue compTOML);

private:
    Scene mScene{};
    TOMLDocument mDoc{};
};

static bool load_rect(Rect& rect, TOMLValue rectTOML);
static void load_transform(TransformEx& transform, TOMLValue transformTOML);
static void load_transform_2d(Transform2D& transform, TOMLValue transformTOML);

static void save_transform(const TransformEx& transform, TOMLWriter writer, const char* key);
static void save_transform_2d(const Transform2D& transform, TOMLWriter writer);

// clang-format off
struct
{
    ComponentType type;
    const char* compTypeName;
    bool (*load)(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName);
    bool (*save)(SceneSchemaSaver& saver, CUID compID, const char* compName);
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

CUID SceneSchemaLoader::load_component(SceneSchemaLoader& loader, TOMLValue compTOML)
{
    if (!loader.mScene || !compTOML || !compTOML.is_table())
        return 0;

    std::string type;
    TOMLValue typeTOML = compTOML["type"];
    if (!typeTOML || !typeTOML.get_string(type))
        return 0;

    std::string name;
    TOMLValue nameTOML = compTOML["name"];
    if (!nameTOML || !nameTOML.get_string(name))
        return 0;

    CUID compID;
    TOMLValue compIDTOML = compTOML["cuid"];
    if (!compIDTOML || !compIDTOML.get_u32(compID))
        return 0;

    for (int i = 1; i < (int)COMPONENT_TYPE_ENUM_COUNT; i++)
    {
        if (type == sSceneSchemaTable[i].compTypeName)
        {
            LD_ASSERT(sSceneSchemaTable[i].load);
            bool ok = sSceneSchemaTable[i].load(loader, compTOML, compID, name.c_str());
            LD_ASSERT(ok); // TODO: deserialization error handling.
        }
    }

    AUID scriptID;
    TOMLValue scriptIDTOML = compTOML["script"];
    if (scriptIDTOML && scriptIDTOML.get_u32(scriptID))
    {
        // the actual script is instantiated later
        loader.mScene.create_component_script_slot(compID, scriptID);
    }

    return compID;
}

bool SceneSchemaLoader::load_audio_source_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName)
{
    Scene scene = loader.mScene;

    compID = scene.create_component(COMPONENT_TYPE_AUDIO_SOURCE, compName, (CUID)0, compID);
    if (!compID)
        return false;

    auto* sourceC = (AudioSourceComponent*)scene.get_component(compID, COMPONENT_TYPE_AUDIO_SOURCE);
    LD_ASSERT(sourceC);

    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.get_u32(sourceC->clipAUID);
    sourceC->playback = {};
    sourceC->pan = 0.5f;
    sourceC->volumeLinear = 1.0f;

    TOMLValue panTOML = compTOML["pan"];
    if (panTOML)
        panTOML.get_f32(sourceC->pan);

    TOMLValue volumeTOML = compTOML["volume_linear"];
    if (volumeTOML)
        volumeTOML.get_f32(sourceC->volumeLinear);

    return true;
}

bool SceneSchemaLoader::load_camera_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName)
{
    Scene scene = loader.mScene;

    compID = scene.create_component(COMPONENT_TYPE_CAMERA, compName, (CUID)0, compID);
    if (!compID)
        return false;

    auto* cameraC = (CameraComponent*)scene.get_component(compID, COMPONENT_TYPE_CAMERA);
    LD_ASSERT(cameraC);

    TOMLValue floatTOML{};
    TOMLValue toml = compTOML["transform"];
    load_transform(cameraC->transform, toml);
    scene.mark_component_transform_dirty(compID);

    toml = compTOML["isPerspective"];
    if (!toml || !toml.get_bool(cameraC->isPerspective))
        return false;

    toml = compTOML["isMainCamera"];
    if (!toml || !toml.get_bool(cameraC->isMainCamera))
        return false;

    if (cameraC->isPerspective)
    {
        toml = compTOML["perspective"];
        if (!toml || !toml.is_table())
            return false; // missing perspective info

        float fovDegrees;
        floatTOML = toml["fov"];
        if (!floatTOML || !floatTOML.get_f32(fovDegrees))
            return false;

        cameraC->perspective.fov = LD_TO_RADIANS(fovDegrees);

        floatTOML = toml["nearClip"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->perspective.nearClip))
            return false;

        floatTOML = toml["farClip"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->perspective.farClip))
            return false;

        // initialized later
        cameraC->perspective.aspectRatio = 0.0f;
    }
    else
    {
        toml = compTOML["orthographic"];
        if (!toml || !toml.is_table())
            return false; // missing orthographic info

        floatTOML = toml["left"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->orthographic.left))
            return false;

        floatTOML = toml["right"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->orthographic.right))
            return false;

        floatTOML = toml["bottom"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->orthographic.bottom))
            return false;

        floatTOML = toml["top"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->orthographic.top))
            return false;

        floatTOML = toml["nearClip"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->orthographic.nearClip))
            return false;

        floatTOML = toml["farClip"];
        if (!floatTOML || !floatTOML.get_f32(cameraC->orthographic.farClip))
            return false;
    }

    return true;
}

bool SceneSchemaLoader::load_mesh_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName)
{
    Scene scene = loader.mScene;

    compID = scene.create_component(COMPONENT_TYPE_MESH, compName, (CUID)0, compID);
    if (!compID)
        return false;

    MeshComponent* meshC = (MeshComponent*)scene.get_component(compID, COMPONENT_TYPE_MESH);
    LD_ASSERT(meshC);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform(meshC->transform, transformTOML);
    scene.mark_component_transform_dirty(compID);

    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.get_u32(meshC->auid);

    return true;
}

bool SceneSchemaLoader::load_sprite_2d_component(SceneSchemaLoader& loader, TOMLValue compTOML, CUID compID, const char* compName)
{
    Scene scene = loader.mScene;

    compID = scene.create_component(COMPONENT_TYPE_SPRITE_2D, compName, (CUID)0, compID);
    if (!compID)
        return false;

    auto* spriteC = (Sprite2DComponent*)scene.get_component(compID, COMPONENT_TYPE_SPRITE_2D);
    LD_ASSERT(spriteC);

    TOMLValue localTOML = compTOML.get_key("local", TOML_TYPE_TABLE);
    if (!load_rect(spriteC->local, localTOML))
        return false;

    TOMLValue transformTOML = compTOML["transform"];
    load_transform_2d(spriteC->transform, transformTOML);
    scene.mark_component_transform_dirty(compID);

    spriteC->auid = 0;
    TOMLValue auidTOML = compTOML["auid"];
    if (auidTOML)
        auidTOML.get_u32(spriteC->auid);

    spriteC->zDepth = 0;
    TOMLValue zDepthTOML = compTOML["zDepth"];
    if (zDepthTOML)
        zDepthTOML.get_u32(spriteC->zDepth);

    return true;
}

static bool load_rect(Rect& rect, TOMLValue rectTOML)
{
    LD_ASSERT(rectTOML && rectTOML.is_table());

    return TOMLUtil::load_rect_table(rect, rectTOML);
}

static void load_transform(TransformEx& transform, TOMLValue transformTOML)
{
    LD_ASSERT(transformTOML && transformTOML.is_table());

    TOMLValue positionTOML = transformTOML["position"];
    LD_ASSERT(positionTOML && positionTOML.is_array() && positionTOML.size() == 3);

    positionTOML[0].get_f32(transform.position.x);
    positionTOML[1].get_f32(transform.position.y);
    positionTOML[2].get_f32(transform.position.z);

    TOMLValue rotationTOML = transformTOML["rotation"];
    LD_ASSERT(rotationTOML && rotationTOML.is_array() && rotationTOML.size() == 3);

    rotationTOML[0].get_f32(transform.rotation.x);
    rotationTOML[1].get_f32(transform.rotation.y);
    rotationTOML[2].get_f32(transform.rotation.z);
    transform.rotation = Quat::from_euler(transform.rotationEuler);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array() && scaleTOML.size() == 3);

    scaleTOML[0].get_f32(transform.scale.x);
    scaleTOML[1].get_f32(transform.scale.y);
    scaleTOML[2].get_f32(transform.scale.z);
}

static void load_transform_2d(Transform2D& transform, TOMLValue transformTOML)
{
    LD_ASSERT(transformTOML && transformTOML.is_table());

    TOMLValue positionTOML = transformTOML["position"];
    LD_ASSERT(positionTOML && positionTOML.is_array() && positionTOML.size() == 2);
    positionTOML[0].get_f32(transform.position.x);
    positionTOML[1].get_f32(transform.position.y);

    TOMLValue rotationTOML = transformTOML["rotation"];
    LD_ASSERT(rotationTOML && rotationTOML.is_float());
    rotationTOML.get_f32(transform.rotation);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array() && scaleTOML.size() == 2);
    scaleTOML[0].get_f32(transform.scale.x);
    scaleTOML[1].get_f32(transform.scale.y);
}

SceneSchemaSaver::~SceneSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool SceneSchemaSaver::save_scene(Scene scene, std::string& toml, std::string& err)
{
    mWriter = TOMLWriter::create();

    mWriter.begin();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MAJOR).value_i32(LD_VERSION_MAJOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MINOR).value_i32(LD_VERSION_MINOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_PATCH).value_i32(LD_VERSION_PATCH);
    mWriter.end_table();

    mWriter.begin_array_table("component");

    Vector<CUID> rootCUIDs;
    mScene.get_root_components(rootCUIDs);
    for (CUID rootCUID : rootCUIDs)
    {
        ComponentBase* rootC = mScene.get_component_base(rootCUID);
        save_component(*this, rootC);
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

bool SceneSchemaSaver::save_audio_source_component(SceneSchemaSaver& saver, CUID compID, const char* compName)
{
    LD_ASSERT(saver.mScene && saver.mWriter && compID && compName);

    auto* sourceC = (AudioSourceComponent*)saver.mScene.get_component(compID, COMPONENT_TYPE_AUDIO_SOURCE);
    if (!sourceC)
        return false;

    TOMLWriter writer = saver.mWriter;
    writer.key("auid").value_u32(sourceC->clipAUID);
    writer.key("pan").value_f32(sourceC->pan);
    writer.key("volumeLinear").value_f32(sourceC->volumeLinear);

    return true;
}

bool SceneSchemaSaver::save_camera_component(SceneSchemaSaver& saver, CUID compID, const char* compName)
{
    LD_ASSERT(saver.mScene && saver.mWriter && compID && compName);

    auto* cameraC = (CameraComponent*)saver.mScene.get_component(compID, COMPONENT_TYPE_CAMERA);
    if (!cameraC)
        return false;

    TOMLWriter writer = saver.mWriter;
    save_transform(cameraC->transform, writer, "transform");

    writer.key("isPerspective").value_bool(cameraC->isPerspective);
    writer.key("isMainCamera").value_bool(cameraC->isMainCamera);

    if (cameraC->isPerspective)
    {
        writer.begin_inline_table("perspective");

        float fovDegrees = (float)LD_TO_DEGREES(cameraC->perspective.fov);
        writer.key("fov").value_f32(fovDegrees);
        writer.key("nearClip").value_f32(cameraC->perspective.nearClip);
        writer.key("farClip").value_f32(cameraC->perspective.farClip);

        writer.end_inline_table();
    }
    else
    {
        writer.begin_inline_table("orthographic");

        writer.key("left").value_f32(cameraC->orthographic.left);
        writer.key("right").value_f32(cameraC->orthographic.right);
        writer.key("bottom").value_f32(cameraC->orthographic.bottom);
        writer.key("top").value_f32(cameraC->orthographic.top);
        writer.key("nearClip").value_f32(cameraC->orthographic.nearClip);
        writer.key("farClip").value_f32(cameraC->orthographic.farClip);

        writer.end_inline_table();
    }

    return true;
}

bool SceneSchemaSaver::save_mesh_component(SceneSchemaSaver& saver, CUID compID, const char* compName)
{
    LD_ASSERT(saver.mScene && saver.mWriter && compID && compName);

    auto* meshC = (MeshComponent*)saver.mScene.get_component(compID, COMPONENT_TYPE_MESH);
    if (!meshC)
        return false;

    TOMLWriter writer = saver.mWriter;
    save_transform(meshC->transform, writer, "transform");
    writer.key("auid").value_u32(meshC->auid);

    return true;
}

bool SceneSchemaSaver::save_sprite_2d_component(SceneSchemaSaver& saver, CUID compID, const char* compName)
{
    LD_ASSERT(saver.mScene && saver.mWriter && compID && compName);

    auto* spriteC = (Sprite2DComponent*)saver.mScene.get_component(compID, COMPONENT_TYPE_SPRITE_2D);
    if (!spriteC)
        return false;

    TOMLWriter writer = saver.mWriter;
    writer.begin_inline_table("local");
    TOMLUtil::save_rect_table(spriteC->local, writer);
    writer.end_inline_table();

    save_transform_2d(spriteC->transform, writer);

    writer.key("auid").value_u32(spriteC->auid);
    writer.key("zDepth").value_u32(spriteC->zDepth);

    return true;
}

void SceneSchemaSaver::save_component(SceneSchemaSaver& saver, ComponentBase* rootC)
{
    // recursively save entire subtree
    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        saver.mChildMap[rootC->id].push_back(childC->id);
        save_component(saver, childC);
    }

    TOMLWriter writer = saver.mWriter;

    writer.begin_table();

    std::string compTypeName(sSceneSchemaTable[(int)rootC->type].compTypeName);
    writer.key("type").value_string(compTypeName);
    writer.key("name").value_string(rootC->name);
    writer.key("cuid").value_u32(rootC->id);

    ComponentScriptSlot* scriptSlot = saver.mScene.get_component_script_slot(rootC->id);
    if (scriptSlot)
        writer.key("script").value_u32(scriptSlot->assetID);

    LD_ASSERT(sSceneSchemaTable[(int)rootC->type].save);
    bool ok = sSceneSchemaTable[(int)rootC->type].save(saver, rootC->id, rootC->name);
    LD_ASSERT(ok); // TODO: error handling path

    writer.end_table();
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
    HashMap<CUID, TOMLValue> compValues;
    TOMLValue componentsTOML = mDoc.get("component");
    if (componentsTOML && componentsTOML.is_array())
    {
        int count = componentsTOML.size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue compTOML = componentsTOML[i];
            if (!compTOML.is_table())
                continue;

            CUID compID;
            TOMLValue compIDTOML = compTOML["cuid"];
            if (compIDTOML && compIDTOML.get_u32(compID))
                compValues[compID] = compTOML;
        }
    }

    for (auto ite : compValues)
    {
        TOMLValue compTOML = ite.second;
        CUID compID = load_component(*this, compTOML);
        LD_ASSERT(compID == ite.first); // TODO: error handling
    }

    TOMLValue hierarchyTOML = mDoc.get("hierarchy");
    if (hierarchyTOML && hierarchyTOML.is_table())
    {
        Vector<std::string> keys;
        hierarchyTOML.get_keys(keys);
        for (const std::string& key : keys)
        {
            CUID parent = static_cast<CUID>(std::stoul(key));
            TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
            if (!childrenTOML || !childrenTOML.is_array())
                continue;

            int count = childrenTOML.size();
            for (int i = 0; i < count; i++)
            {
                CUID child;
                if (!childrenTOML[i].get_u32(child))
                    continue;

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
    if (!FS::read_file_to_vector(tomlPath, toml))
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

    return FS::write_file_and_swap_backup(savePath, toml.size(), (const byte*)toml.data(), err);
}

} // namespace LD
