#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Format/JSON.h>
#include <Ludens/RenderServer/RServer.h>
#include <vector>

namespace LD {

/// @brief Scene creation info
struct SceneInfo
{
    JSONDocument jsonDoc;
    AssetManager assetManager;
    RServer renderServer;
};

/// @brief The basic unit of game simulation.
///        A scene is a hierarchy of components driven by scripts.
struct Scene : Handle<struct SceneObj>
{
public:
    /// @brief Create scene from JSON declaration and Assets
    static Scene create(const SceneInfo& info);

    /// @brief Destroy a Scene
    static void destroy(Scene);

    /// @brief Get root components in Scene
    void get_root_components(std::vector<DUID>& roots);

    /// @brief Get data component base members
    const DataComponent* get_component_base(DUID compID);

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