#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/ComponentViews.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/Serial/Property.h>

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

    bool save_scene(Scene scene, String& toml, String& err);

private:
    static void save_subtree(SceneSchemaSaver& saver, ComponentView compV);

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

    bool load_scene(Scene scene, SUIDRegistry idReg, const View& toml, String& err);

private:
    static ComponentView load(SceneSchemaLoader& loader, String& err);

private:
    Scene mScene{};
    TOMLReader mReader{};
    SUIDRegistry mIDReg{};
};

ComponentView SceneSchemaLoader::load(SceneSchemaLoader& loader, String& err)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;
    LD_ASSERT(scene && reader);

    if (!reader.is_table_scope())
        return {};

    String type;
    if (!reader.read_string(SCENE_SCHEMA_KEY_COMPONENT_TYPE, type))
        return {};

    String compName;
    if (!reader.read_string(SCENE_SCHEMA_KEY_COMPONENT_NAME, compName))
        return {};

    SUID compSUID;
    if (!reader.read_suid(SCENE_SCHEMA_KEY_COMPONENT_ID, compSUID))
    {
        err = std::format("component missing ID field");
        return {};
    }

    if (compSUID.type() != SERIAL_TYPE_COMPONENT)
    {
        err = std::format("component invalid SUID {}", compSUID);
        return {};
    }

    ComponentView compV{};

    for (int i = 1; i < (int)COMPONENT_TYPE_ENUM_COUNT; i++)
    {
        ComponentType compType = (ComponentType)i;

        if (type == get_component_brief_type_name(compType))
        {
            const TypeMeta* compM = ComponentView::type_meta(compType);

            // TODO: error handling
            compV = scene.create_component_serial(compType, compName, loader.mIDReg, (SUID)0, compSUID);
            LD_ASSERT(compV);

            Vector<PropertyValue> props;
            (void)TOMLUtil::read_type_meta(reader, compM, props);
            bool ok = compV.load_from_props(props, err);
            LD_ASSERT(ok);
        }
    }

    AssetID scriptID = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_COMPONENT_SCRIPT_ID, scriptID);

    compV.set_script_asset_id(scriptID);

    return compV;
}

SceneSchemaSaver::~SceneSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool SceneSchemaSaver::save_scene(Scene scene, String& toml, String& err)
{
    mScene = scene;

    mWriter = TOMLWriter::create();

    mWriter.begin();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MAJOR).write_i32(LD_VERSION_MAJOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MINOR).write_i32(LD_VERSION_MINOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_PATCH).write_i32(LD_VERSION_PATCH);
    mWriter.end_table();

    mWriter.begin_array_table(SCENE_SCHEMA_TABLE_COMPONENT);

    Vector<ComponentView> roots;
    mScene.get_root_components(roots);
    for (ComponentView root : roots)
    {
        save_subtree(*this, root);
    }

    mWriter.end_array_table();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_HIERARCHY);
    for (auto ite : mChildMap)
    {
        std::string parentID = std::to_string((uint32_t)ite.first);
        mWriter.key(parentID.c_str()).begin_array();

        for (SUID childrenID : ite.second)
            mWriter.write_u32(childrenID);

        mWriter.end_array();
    }
    mWriter.end_table();
    mWriter.end(toml);

    TOMLWriter::destroy(mWriter);
    mWriter = {};

    return true;
}

void SceneSchemaSaver::save_subtree(SceneSchemaSaver& saver, ComponentView compV)
{
    LD_ASSERT(compV);

    TOMLWriter writer = saver.mWriter;

    writer.begin_table();
    {
        View typeNameV = get_component_brief_type_name(compV.type());
        String typeName((const char*)typeNameV.data, typeNameV.size);

        writer.key(SCENE_SCHEMA_KEY_COMPONENT_ID).write_u32(compV.suid());
        writer.key(SCENE_SCHEMA_KEY_COMPONENT_TYPE).write_string(typeName);
        writer.key(SCENE_SCHEMA_KEY_COMPONENT_NAME).write_string(compV.get_name());
        writer.key(SCENE_SCHEMA_KEY_COMPONENT_SCRIPT_ID).write_u32(compV.get_script_asset_id());

        TOMLUtil::write_type_meta(writer, compV.type_meta(), compV.data());
    }
    writer.end_table();

    // recursively save entire subtree
    Vector<ComponentView> children;
    compV.get_children(children);

    for (ComponentView child : children)
    {
        LD_ASSERT(child);
        saver.mChildMap[compV.suid()].push_back(child.suid());
        save_subtree(saver, child);
    }
}

SceneSchemaLoader::~SceneSchemaLoader()
{
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool SceneSchemaLoader::load_scene(Scene scene, SUIDRegistry idReg, const View& toml, String& err)
{
    mScene = scene;
    mReader = TOMLReader::create(toml, err);
    mIDReg = idReg;

    if (!mReader)
        return false;

    scene.reset();

    if (!mReader.enter_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE))
        return false;

    uint32_t version;
    if (!mReader.read_u32(SCENE_SCHEMA_KEY_VERSION_MAJOR, version) || version != LD_VERSION_MAJOR)
        return false;

    if (!mReader.read_u32(SCENE_SCHEMA_KEY_VERSION_MINOR, version) || version != LD_VERSION_MINOR)
        return false;

    if (!mReader.read_u32(SCENE_SCHEMA_KEY_VERSION_PATCH, version) || version != LD_VERSION_PATCH)
        return false;

    mReader.exit();

    bool valid = true;

    // extract component tables
    int componentCount = 0;
    if (mReader.enter_array(SCENE_SCHEMA_TABLE_COMPONENT, componentCount))
    {
        for (int i = 0; valid && i < componentCount; i++)
        {
            if (!mReader.enter_table(i))
                continue;

            ComponentView comp = load(*this, err);
            if (!comp)
                valid = false;

            mReader.exit();
        }

        mReader.exit();
    }

    if (valid && mReader.enter_table(SCENE_SCHEMA_TABLE_HIERARCHY))
    {
        Vector<String> keys;
        mReader.get_keys(keys);

        for (const String& key : keys)
        {
            uint32_t u32;
            std::from_chars((const char*)key.data(), (const char*)key.data() + key.size(), u32);
            SUID parentSUID(u32);

            if (parentSUID.type() != SERIAL_TYPE_COMPONENT)
            {
                err = std::format("found invalid component SUID {}", parentSUID);
                valid = false;
                continue;
            }

            int childrenCount = 0;
            if (!mReader.enter_array(key.c_str(), childrenCount))
                continue;

            for (int i = 0; i < childrenCount; i++)
            {
                SUID childSUID;
                if (!mReader.read_suid(i, childSUID))
                    continue;

                if (childSUID.type() != SERIAL_TYPE_COMPONENT)
                {
                    err = std::format("found invalid component SUID {}", childSUID);
                    valid = false;
                    continue;
                }

                ComponentView child = mScene.get_component_by_suid(childSUID);
                ComponentView parent = mScene.get_component_by_suid(parentSUID);

                if (child && parent)
                    scene.reparent_component_subtree(child.cuid(), parent.cuid());
            }

            mReader.exit();
        }

        mReader.exit();
    }

    TOMLReader::destroy(mReader);
    mReader = {};

    return valid;
}

//
// Public API
//

bool SceneSchema::load_scene_from_source(Scene scene, SUIDRegistry idRegistry, const View& toml, String& err)
{
    LD_PROFILE_SCOPE;

    SceneSchemaLoader loader;
    if (!loader.load_scene(scene, idRegistry, toml, err))
        return false;

    return true;
}

bool SceneSchema::load_scene_from_file(Scene scene, SUIDRegistry idRegistry, const FS::Path& tomlPath, String& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    return load_scene_from_source(scene, idRegistry, view(toml), err);
}

bool SceneSchema::save_scene(Scene scene, const FS::Path& savePath, String& err)
{
    LD_PROFILE_SCOPE;

    String toml;
    SceneSchemaSaver saver;
    if (!saver.save_scene(scene, toml, err))
        return false;

    return FS::write_file_and_swap_backup(savePath, view(toml), err);
}

String SceneSchema::create_empty()
{
    TOMLWriter writer = TOMLWriter::create();

    writer.begin();

    writer.begin_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    writer.key(SCENE_SCHEMA_KEY_VERSION_MAJOR).write_i32(LD_VERSION_MAJOR);
    writer.key(SCENE_SCHEMA_KEY_VERSION_MINOR).write_i32(LD_VERSION_MINOR);
    writer.key(SCENE_SCHEMA_KEY_VERSION_PATCH).write_i32(LD_VERSION_PATCH);
    writer.end_table();

    String toml;
    writer.end(toml);

    TOMLWriter::destroy(writer);

    return toml;
}

} // namespace LD
