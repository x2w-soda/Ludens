#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/Memory.h>
#include <cstdint>
#include <format>
#include <unordered_map>

namespace LD {

/// @brief Helper to retain some hierarchical context when saving a Scene.
struct SceneSaver
{
    Scene scene;
    TOMLDocument doc;
    std::vector<CUID> rootCUIDs;
    std::unordered_map<CUID, std::vector<CUID>> childMap;
    TOMLValue compArrayTOML{};

    SceneSaver() = delete;
    SceneSaver(Scene scene, TOMLDocument doc)
        : scene(scene), doc(doc)
    {
        LD_ASSERT(scene && doc);
        scene.get_root_components(rootCUIDs);
        compArrayTOML = doc.set("component", TOML_TYPE_ARRAY);
    }

    /// @brief Get a new table value in the [[component]] TOML array.
    inline TOMLValue get_component_toml()
    {
        return compArrayTOML.append(TOML_TYPE_TABLE);
    }
};

static void load_scene_from_schema(Scene scene, TOMLDocument doc);
static CUID load_component(Scene scene, TOMLValue compTOML);
static bool load_audio_source_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static bool load_camera_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static bool load_mesh_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static bool load_sprite_2d_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static void load_transform(Transform& transform, TOMLValue transformTOML);
static void load_transform_2d(Transform2D& transform, TOMLValue transformTOML);

static void save_scene_to_schema(Scene scene, TOMLDocument doc);
static void save_component(SceneSaver& saver, ComponentBase* rootC);
static bool save_audio_source_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static bool save_camera_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static bool save_mesh_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static bool save_sprite_2d_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
static void save_transform(const Transform& transform, TOMLValue transformTOML);
static void save_transform_2d(const Transform2D& transform, TOMLValue transformTOML);

// clang-format off
struct
{
    ComponentType type;
    const char* compTypeName;
    bool (*load)(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
    bool (*save)(Scene scene, TOMLValue compTOML, CUID compID, const char* compName);
} sSceneSchemaTable[] = {
    {COMPONENT_TYPE_DATA,           "Data",        nullptr,                        nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE,   "AudioSource", &load_audio_source_component,   &save_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,      "Transform",   nullptr,                        nullptr},
    {COMPONENT_TYPE_CAMERA,         "Camera",      &load_camera_component,         &save_camera_component},
    {COMPONENT_TYPE_MESH,           "Mesh",        &load_mesh_component,           &save_mesh_component},
    {COMPONENT_TYPE_SPRITE_2D,      "Sprite2D",    &load_sprite_2d_component,      &save_sprite_2d_component},
};
// clang-format on

static_assert(sizeof(sSceneSchemaTable) / sizeof(*sSceneSchemaTable) == COMPONENT_TYPE_ENUM_COUNT);

static void load_scene_from_schema(Scene scene, TOMLDocument doc)
{
    if (!scene || !doc)
        return;

    TOMLValue sceneTOML = doc.get("ludens_scene");
    if (!sceneTOML || sceneTOML.get_type() != TOML_TYPE_TABLE)
        return;

    int32_t version;
    TOMLValue versionTOML = sceneTOML["version_major"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MAJOR)
        return;

    versionTOML = sceneTOML["version_minor"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MINOR)
        return;

    versionTOML = sceneTOML["version_patch"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_PATCH)
        return;

    // extract component tables
    std::unordered_map<CUID, TOMLValue> compValues;
    TOMLValue componentsTOML = doc.get("component");
    if (componentsTOML && componentsTOML.is_array_type())
    {
        int count = componentsTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue compTOML = componentsTOML[i];
            if (!compTOML.is_table_type())
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
        CUID compID = load_component(scene, compTOML);
        LD_ASSERT(compID == ite.first); // TODO: error handling
    }

    TOMLValue hierarchyTOML = doc.get("hierarchy");
    if (!hierarchyTOML || !hierarchyTOML.is_table_type())
        return;

    std::vector<std::string> keys;
    hierarchyTOML.get_keys(keys);
    for (const std::string& key : keys)
    {
        CUID parent = static_cast<CUID>(std::stoul(key));
        TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
        if (!childrenTOML || !childrenTOML.is_array_type())
            continue;

        int count = childrenTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            CUID child;
            if (!childrenTOML[i].get_u32(child))
                continue;

            scene.reparent(child, parent);
        }
    }
}

static CUID load_component(Scene scene, TOMLValue compTOML)
{
    if (!compTOML || !scene || !compTOML.is_table_type())
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
            bool ok = sSceneSchemaTable[i].load(scene, compTOML, compID, name.c_str());
            LD_ASSERT(ok); // TODO: deserialization error handling.
        }
    }

    AUID scriptID;
    TOMLValue scriptIDTOML = compTOML["script"];
    if (scriptIDTOML && scriptIDTOML.get_u32(scriptID))
    {
        // the actual script is instantiated later
        scene.create_component_script_slot(compID, scriptID);
    }

    return compID;
}

static bool load_audio_source_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_AUDIO_SOURCE, compName, (CUID)0, compID);
    if (!compID)
        return false;

    AudioSourceComponent* sourceC = (AudioSourceComponent*)scene.get_component(compID, type);

    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.get_u32(sourceC->clipAUID);
    sourceC->playback = {};

    return true;
}

static bool load_camera_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_CAMERA, compName, (CUID)0, compID);
    if (!compID)
        return false;

    CameraComponent* cameraC = (CameraComponent*)scene.get_component(compID, type);

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
        if (!toml || !toml.is_table_type())
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
        if (!toml || !toml.is_table_type())
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

static bool load_mesh_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_MESH, compName, (CUID)0, compID);
    if (!compID)
        return false;

    MeshComponent* meshC = (MeshComponent*)scene.get_component(compID, type);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform(meshC->transform, transformTOML);
    scene.mark_component_transform_dirty(compID);

    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.get_u32(meshC->auid);

    return true;
}

static bool load_sprite_2d_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_SPRITE_2D, compName, (CUID)0, compID);
    if (!compID)
        return false;

    Sprite2DComponent* spriteC = (Sprite2DComponent*)scene.get_component(compID, type);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform_2d(spriteC->transform, transformTOML);
    scene.mark_component_transform_dirty(compID);

    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.get_u32(spriteC->auid);

    return true;
}

static void load_transform(Transform& transform, TOMLValue transformTOML)
{
    LD_ASSERT(transformTOML && transformTOML.is_table_type());

    TOMLValue positionTOML = transformTOML["position"];
    LD_ASSERT(positionTOML && positionTOML.is_array_type() && positionTOML.get_size() == 3);

    positionTOML[0].get_f32(transform.position.x);
    positionTOML[1].get_f32(transform.position.y);
    positionTOML[2].get_f32(transform.position.z);

    TOMLValue rotationTOML = transformTOML["rotation"];
    LD_ASSERT(rotationTOML && rotationTOML.is_array_type() && rotationTOML.get_size() == 3);

    rotationTOML[0].get_f32(transform.rotation.x);
    rotationTOML[1].get_f32(transform.rotation.y);
    rotationTOML[2].get_f32(transform.rotation.z);
    transform.quat = Quat::from_euler(transform.rotation);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array_type() && scaleTOML.get_size() == 3);

    scaleTOML[0].get_f32(transform.scale.x);
    scaleTOML[1].get_f32(transform.scale.y);
    scaleTOML[2].get_f32(transform.scale.z);
}

static void load_transform_2d(Transform2D& transform, TOMLValue transformTOML)
{
    LD_ASSERT(transformTOML && transformTOML.is_table_type());

    TOMLValue positionTOML = transformTOML["position"];
    LD_ASSERT(positionTOML && positionTOML.is_array_type() && positionTOML.get_size() == 2);
    positionTOML[0].get_f32(transform.position.x);
    positionTOML[1].get_f32(transform.position.y);

    TOMLValue rotationTOML = transformTOML["rotation"];
    LD_ASSERT(rotationTOML && rotationTOML.is_float_type());
    rotationTOML.get_f32(transform.rotation);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array_type() && scaleTOML.get_size() == 2);
    scaleTOML[0].get_f32(transform.scale.x);
    scaleTOML[1].get_f32(transform.scale.y);
}

static void save_scene_to_schema(Scene scene, TOMLDocument doc)
{
    SceneSaver saver(scene, doc);

    TOMLValue sceneTOML = doc.set("ludens_scene", TOML_TYPE_TABLE);
    sceneTOML.set_key("version_major", TOML_TYPE_INT).set_i32(LD_VERSION_MAJOR);
    sceneTOML.set_key("version_minor", TOML_TYPE_INT).set_i32(LD_VERSION_MINOR);
    sceneTOML.set_key("version_patch", TOML_TYPE_INT).set_i32(LD_VERSION_PATCH);

    for (CUID rootCUID : saver.rootCUIDs)
    {
        ComponentBase* rootC = scene.get_component_base(rootCUID);
        save_component(saver, rootC);
    }

    TOMLValue hierarchyTOML = doc.set("hierarchy", TOML_TYPE_TABLE);
    for (auto ite : saver.childMap)
    {
        std::string parentID = std::to_string(ite.first);

        TOMLValue childrenTOML = hierarchyTOML.set_key(parentID.c_str(), TOML_TYPE_ARRAY);
        for (CUID childrenID : ite.second)
        {
            childrenTOML.append(TOML_TYPE_INT).set_u32(childrenID);
        }
    }
}

static void save_component(SceneSaver& saver, ComponentBase* rootC)
{
    // recursively save entire subtree
    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        saver.childMap[rootC->id].push_back(childC->id);
        save_component(saver, childC);
    }

    TOMLValue compTOML = saver.get_component_toml();
    std::string compTypeName(sSceneSchemaTable[(int)rootC->type].compTypeName);
    compTOML.set_key("type", TOML_TYPE_STRING).set_string(compTypeName);
    compTOML.set_key("name", TOML_TYPE_STRING).set_string(rootC->name);
    compTOML.set_key("cuid", TOML_TYPE_INT).set_u32(rootC->id);

    LD_ASSERT(sSceneSchemaTable[(int)rootC->type].save);
    bool ok = sSceneSchemaTable[(int)rootC->type].save(saver.scene, compTOML, rootC->id, rootC->name);
    LD_ASSERT(ok); // TODO: error handling path

    ComponentScriptSlot* scriptSlot = saver.scene.get_component_script_slot(rootC->id);
    if (scriptSlot)
    {
        compTOML.set_key("script", TOML_TYPE_INT).set_u32(scriptSlot->assetID);
    }
}

static bool save_audio_source_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    LD_ASSERT(scene && compTOML && compID && compName);

    ComponentType type;
    AudioSourceComponent* sourceC = (AudioSourceComponent*)scene.get_component(compID, type);
    if (type != COMPONENT_TYPE_AUDIO_SOURCE)
        return false;

    TOMLValue auidTOML = compTOML.set_key("auid", TOML_TYPE_INT);
    auidTOML.set_u32(sourceC->clipAUID);

    return true;
}

static bool save_camera_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    LD_ASSERT(scene && compTOML && compID && compName);

    ComponentType type;
    const CameraComponent* cameraC = (CameraComponent*)scene.get_component(compID, type);
    if (type != COMPONENT_TYPE_CAMERA)
        return false;

    TOMLValue floatTOML{};
    TOMLValue transformTOML = compTOML.set_key("transform", TOML_TYPE_TABLE);
    save_transform(cameraC->transform, transformTOML);

    compTOML.set_key("isPerspective", TOML_TYPE_BOOL).set_bool(cameraC->isPerspective);
    compTOML.set_key("isMainCamera", TOML_TYPE_BOOL).set_bool(cameraC->isMainCamera);

    if (cameraC->isPerspective)
    {
        TOMLValue perspectiveTOML = compTOML.set_key("perspective", TOML_TYPE_TABLE);
        perspectiveTOML.format(TOML_FORMAT_TABLE_ONE_LINE);

        float fovDegrees = (float)LD_TO_DEGREES(cameraC->perspective.fov);
        perspectiveTOML.set_key("fov", TOML_TYPE_FLOAT).set_f32(fovDegrees);
        perspectiveTOML.set_key("nearClip", TOML_TYPE_FLOAT).set_f32(cameraC->perspective.nearClip);
        perspectiveTOML.set_key("farClip", TOML_TYPE_FLOAT).set_f32(cameraC->perspective.farClip);
    }
    else
    {
        TOMLValue orthoTOML = compTOML.set_key("orthographic", TOML_TYPE_TABLE);
        orthoTOML.format(TOML_FORMAT_TABLE_ONE_LINE);

        orthoTOML.set_key("left", TOML_TYPE_FLOAT).set_f32(cameraC->orthographic.left);
        orthoTOML.set_key("right", TOML_TYPE_FLOAT).set_f32(cameraC->orthographic.right);
        orthoTOML.set_key("bottom", TOML_TYPE_FLOAT).set_f32(cameraC->orthographic.bottom);
        orthoTOML.set_key("top", TOML_TYPE_FLOAT).set_f32(cameraC->orthographic.top);
        orthoTOML.set_key("nearClip", TOML_TYPE_FLOAT).set_f32(cameraC->orthographic.nearClip);
        orthoTOML.set_key("farClip", TOML_TYPE_FLOAT).set_f32(cameraC->orthographic.farClip);
    }

    return true;
}

static bool save_mesh_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    LD_ASSERT(scene && compTOML && compID && compName);

    ComponentType type;
    MeshComponent* meshC = (MeshComponent*)scene.get_component(compID, type);
    if (type != COMPONENT_TYPE_MESH)
        return false;

    TOMLValue transformTOML = compTOML.set_key("transform", TOML_TYPE_TABLE);
    save_transform(meshC->transform, transformTOML);

    TOMLValue auidTOML = compTOML.set_key("auid", TOML_TYPE_INT);
    auidTOML.set_u32(meshC->auid);

    return true;
}

static bool save_sprite_2d_component(Scene scene, TOMLValue compTOML, CUID compID, const char* compName)
{
    LD_ASSERT(scene && compTOML && compID && compName);

    ComponentType type;
    Sprite2DComponent* spriteC = (Sprite2DComponent*)scene.get_component(compID, type);
    if (type != COMPONENT_TYPE_SPRITE_2D)
        return false;

    TOMLValue transformTOML = compTOML.set_key("transform", TOML_TYPE_TABLE);
    save_transform_2d(spriteC->transform, transformTOML);

    TOMLValue auidTOML = compTOML.set_key("auid", TOML_TYPE_INT);
    auidTOML.set_u32(spriteC->auid);

    return true;
}

static void save_transform(const Transform& transform, TOMLValue transformTOML)
{
    LD_ASSERT(transformTOML && transformTOML.is_table_type());

    transformTOML.format(TOML_FORMAT_TABLE_ONE_LINE);

    TOMLValue positionTOML = transformTOML.set_key("position", TOML_TYPE_ARRAY);
    positionTOML.append(TOML_TYPE_FLOAT).set_f32(transform.position.x);
    positionTOML.append(TOML_TYPE_FLOAT).set_f32(transform.position.y);
    positionTOML.append(TOML_TYPE_FLOAT).set_f32(transform.position.z);

    TOMLValue rotationTOML = transformTOML.set_key("rotation", TOML_TYPE_ARRAY);
    rotationTOML.append(TOML_TYPE_FLOAT).set_f32(transform.rotation.x);
    rotationTOML.append(TOML_TYPE_FLOAT).set_f32(transform.rotation.y);
    rotationTOML.append(TOML_TYPE_FLOAT).set_f32(transform.rotation.z);

    TOMLValue scaleTOML = transformTOML.set_key("scale", TOML_TYPE_ARRAY);
    scaleTOML.append(TOML_TYPE_FLOAT).set_f32(transform.scale.x);
    scaleTOML.append(TOML_TYPE_FLOAT).set_f32(transform.scale.y);
    scaleTOML.append(TOML_TYPE_FLOAT).set_f32(transform.scale.z);
}

static void save_transform_2d(const Transform2D& transform, TOMLValue transformTOML)
{
    LD_ASSERT(transformTOML && transformTOML.is_table_type());

    transformTOML.format(TOML_FORMAT_TABLE_ONE_LINE);

    TOMLValue positionTOML = transformTOML.set_key("position", TOML_TYPE_ARRAY);
    positionTOML.append(TOML_TYPE_FLOAT).set_f32(transform.position.x);
    positionTOML.append(TOML_TYPE_FLOAT).set_f32(transform.position.y);

    TOMLValue rotationTOML = transformTOML.set_key("rotation", TOML_TYPE_FLOAT);
    rotationTOML.set_f32(transform.rotation);

    TOMLValue scaleTOML = transformTOML.set_key("scale", TOML_TYPE_ARRAY);
    scaleTOML.append(TOML_TYPE_FLOAT).set_f32(transform.scale.x);
    scaleTOML.append(TOML_TYPE_FLOAT).set_f32(transform.scale.y);
}

//
// Public API
//

void SceneSchema::load_scene_from_source(Scene scene, const char* source, size_t len)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create();

    std::string err;
    bool success = doc.parse(source, len, err);
    if (!success)
    {
        TOMLDocument::destroy(doc);
        return; // TODO: error handling path
    }

    load_scene_from_schema(scene, doc);
    TOMLDocument::destroy(doc);
}

void SceneSchema::load_scene_from_file(Scene scene, const FS::Path& tomlPath)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create_from_file(tomlPath);
    load_scene_from_schema(scene, doc);
    TOMLDocument::destroy(doc);
}

bool SceneSchema::save_scene(Scene scene, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create();
    save_scene_to_schema(scene, doc);

    std::string str;
    if (!doc.save_to_string(str))
    {
        TOMLDocument::destroy(doc);
        return false;
    }

    TOMLDocument::destroy(doc);
    return FS::write_file_and_swap_backup(savePath, str.size(), (const byte*)str.data(), err);
}

std::string SceneSchema::get_default_text()
{
    return std::format(R"(
[ludens_scene]
version_major = {}
version_minor = {}
version_patch = {}
)",
                       LD_VERSION_MAJOR,
                       LD_VERSION_MINOR,
                       LD_VERSION_PATCH);
}

} // namespace LD
