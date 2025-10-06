#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Scene/Scene.h>
#include <cstdint>
#include <string>

namespace LD {

/// @brief Schema for defining a Scene under the current framework version.
struct SceneSchema : Handle<struct SceneSchemaObj>
{
    /// @brief Create schema from TOML source string.
    static SceneSchema create_from_source(const char* source, size_t len);

    /// @brief Create schema from TOML file on disk.
    static SceneSchema create_from_file(const FS::Path& tomlPath);

    /// @brief Destroy the scene schema.
    static void destroy(SceneSchema schema);

    /// @brief Get default schema TOML text.
    static std::string get_default_text();

    /// @brief Populate a scene from schema. Assets are not loaded yet but
    ///        the component hierarchy and asset IDs should be in place.
    void load_scene(Scene scene);

    /// @brief Write TOML schema to disk.
    /// @param savePath Path to save the schema.
    /// @return True on success.
    bool save_to_disk(const FS::Path& savePath);

    /// @brief Get script asset ID associated with component, or 0 if not found.
    AUID get_component_script(CUID compID);

    /// @brief Set script ID associated with component.
    void set_component_script(CUID compID, AUID scriptAssetID);
};

} // namespace LD