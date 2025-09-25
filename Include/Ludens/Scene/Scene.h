#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderServer/RServer.h>
#include <vector>

namespace LD {

/// @brief Scene preparation info
struct ScenePrepareInfo
{
    AssetManager assetManager;
    RServer renderServer;
};

/// @brief The basic unit of game simulation.
///        A scene is a hierarchy of components driven by scripts.
struct Scene : Handle<struct SceneObj>
{
public:

    /// @brief Create empty scene with no components.
    static Scene create();

    /// @brief Destroy a Scene.
    static void destroy(Scene);

    /// @brief Prepare the scene. The AssetManager loads all assets used by the scene.
    void prepare(const ScenePrepareInfo& info);

    /// @brief Startup the scene for simulation. This attaches scripts to their components.
    void startup();

    /// @brief Cleanup the scene simulation. This detaches scripts from their components.
    void cleanup();

    /// @brief Duplicate the current scene into the backup scene.
    void backup();

    /// @brief Swap current scene and backup scene.
    void swap();

    /// @brief Update the scene with delta time.
    /// @param delta Delta time in seconds.
    void update(float delta);

    /// @brief Create a component.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param parent Parent component, or zero if creating a root component.
    /// @param hint If not zero, hint ID to create component with. 
    CUID create_component(ComponentType type, const char* name, CUID parent, CUID hint);

    /// @brief Create data component script slot.
    /// @param compID Component ID.
    /// @param assetID ScriptAsset ID.
    /// @return Script slot for the component.
    ComponentScriptSlot* create_component_script_slot(CUID compID, AUID assetID);

    /// @brief Destroy a component.
    void destroy_component(CUID compID);

    /// @brief Reparent a component
    void reparent(CUID compID, CUID parentID);

    /// @brief Get root components in Scene
    void get_root_components(std::vector<CUID>& roots);

    /// @brief Get data component base members
    ComponentBase* get_component_base(CUID compID);

    /// @brief Get data component script slot, or null if not found.
    ComponentScriptSlot* get_component_script_slot(CUID compID);

    /// @brief Get data component
    void* get_component(CUID compID, ComponentType& type);

    /// @brief Lookup some render server ID for data component.
    ///        Only graphical components such as Meshes are applicable.
    RUID get_component_ruid(CUID compID);

    /// @brief Get local transform of a data component.
    bool get_component_transform(CUID compID, Transform& transform);

    /// @brief Get local transform of a data component.
    bool set_component_transform(CUID compID, const Transform& transform);

    /// @brief Get component world matrix.
    bool get_component_transform_mat4(CUID compID, Mat4& worldMat4);

    /// @brief Mark the transforms of a component subtree as dirty.
    void mark_component_transform_dirty(CUID compID);

    /// @brief Lookup the data component from draw call ID
    CUID get_ruid_component(RUID ruid);

    /// @brief Supplies the transform for a draw call
    Mat4 get_ruid_transform_mat4(RUID ruid);

    void set_mesh_component_asset(CUID meshC, AUID meshAssetID);
};

} // namespace LD