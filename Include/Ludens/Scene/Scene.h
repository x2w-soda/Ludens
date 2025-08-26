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

    /// @brief Update the scene with delta time.
    /// @param delta Delta time in seconds.
    void update(float delta);

    /// @brief Create a component.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param parent Parent component, or zero if creating a root component.
    DUID create_component(ComponentType type, const char* name, DUID parent);

    /// @brief Create data component script slot.
    /// @param compID Component ID.
    /// @param assetID ScriptAsset ID.
    /// @return Script slot for the component.
    ComponentScriptSlot* create_component_script_slot(DUID compID, AUID assetID);

    /// @brief Destroy a component.
    void destroy_component(DUID compID);

    /// @brief Get root components in Scene
    void get_root_components(std::vector<DUID>& roots);

    /// @brief Get data component base members
    ComponentBase* get_component_base(DUID compID);

    /// @brief Get data component
    void* get_component(DUID compID, ComponentType& type);

    /// @brief Lookup some render server ID for data component.
    ///        Only graphical components such as Meshes are applicable.
    RUID get_component_ruid(DUID compID);

    /// @brief Get address of component transform or nullptr
    Transform* get_component_transform(DUID compID);

    /// @brief Lookup the data component from some render server ID
    DUID get_ruid_component(RUID ruid);

    /// @brief Supplies the transform for the render server
    Mat4 get_ruid_transform(RUID ruid);
};

} // namespace LD