#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/AudioServer/AudioServer.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderServer/RenderServer.h>
#include <vector>

namespace LD {

/// @brief Get static C string of log channel used by lua scripts.
const char* get_lua_script_log_channel_name();

/// @brief Scene creation info, connects to external asset manager and subsystems.
struct SceneInfo
{
    AssetManager assetManager;
    AudioServer audioServer;
    RenderServer renderServer;
};

/// @brief The basic unit of game simulation.
///        A scene is a hierarchy of components driven by scripts.
struct Scene : Handle<struct SceneObj>
{
public:
    /// @brief Create empty scene with no components.
    static Scene create(const SceneInfo& sceneI);

    /// @brief Destroy a Scene.
    static void destroy(Scene);

    /// @brief In-place reset to initial state after creation.
    void reset();

    /// @brief Load the scene. Creates resources from assets and subsystems.
    void load();

    /// @brief Unload the scene. Destroys resouorces.
    void unload();

    /// @brief Startup the scene for simulation. This attaches scripts to their components.
    void startup();

    /// @brief Cleanup the scene simulation. This detaches scripts from their components.
    void cleanup();

    /// @brief Duplicate the current scene into the backup scene.
    void backup();

    /// @brief Swap current scene and backup scene.
    void swap();

    /// @brief Update the scene with delta time.
    /// @param screenExtent Screen size this frame.
    /// @param delta Delta time in seconds.
    void update(const Vec2& screenExtent, float delta);

    /// @brief Get camera to render the Scene with.
    Camera get_camera();

    /// @brief Create a component.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param parent Parent component, or zero if creating a root component.
    /// @param hint If not zero, hint ID to create component with.
    CUID create_component(ComponentType type, const char* name, CUID parent, CUID hint);

    /// @brief Destroy a component.
    void destroy_component(CUID compID);

    /// @brief Create data component script slot.
    /// @param compID Component ID.
    /// @param assetID ScriptAsset ID.
    /// @return Script slot for the component.
    ComponentScriptSlot* create_component_script_slot(CUID compID, AUID assetID);

    /// @brief Destroy data component script slot if it exists.
    void destroy_component_script_slot(CUID compID);

    /// @brief Reparent a component
    void reparent(CUID compID, CUID parentID);

    /// @brief Get root components in Scene
    void get_root_components(std::vector<CUID>& roots);

    /// @brief Get data component base members
    ComponentBase* get_component_base(CUID compID);

    /// @brief Get data component script slot, or null if not found.
    ComponentScriptSlot* get_component_script_slot(CUID compID);

    /// @brief Get data component from ID.
    void* get_component(CUID compID, ComponentType* outType);

    /// @brief Get data component from ID and expected type, fails upon type mismatch.
    void* get_component(CUID compID, ComponentType expectedType);

    /// @brief Lookup some render server ID for data component.
    ///        Only graphical components such as Meshes are applicable.
    RUID get_component_ruid(CUID compID);

    /// @brief Get local transform of a data component.
    bool get_component_transform(CUID compID, TransformEx& transform);

    /// @brief Get local transform of a data component.
    bool set_component_transform(CUID compID, const TransformEx& transform);

    /// @brief Get local 2D transform of a data component.
    bool get_component_transform_2d(CUID compID, Transform2D& transform);

    /// @brief Get component world matrix.
    bool get_component_world_mat4(CUID compID, Mat4& worldMat4);

    /// @brief Mark the transforms of a component subtree as dirty.
    void mark_component_transform_dirty(CUID compID);

    /// @brief Lookup the data component from draw call ID
    CUID get_ruid_component(RUID ruid);

    /// @brief Supplies the Mat4 model matrix for a draw call
    Mat4 get_ruid_transform_mat4(RUID ruid);

    /// @brief Public interface for audio source components.
    class AudioSource
    {
    public:
        AudioSource() = delete;
        AudioSource(AudioSourceComponent* comp);

        inline operator bool() const noexcept { return mComp && mComp->playback; }

        void play();
        void pause();
        void resume();

        void set_clip_asset(AUID clipAUID);

    private:
        AudioSourceComponent* mComp;
        CUID mCUID;
    };

    /// @brief Public interface for mesh components.
    class Mesh
    {
    public:
        Mesh() = delete;
        Mesh(MeshComponent* comp);

        inline operator bool() const noexcept { return mComp && mComp->draw; }

        void set_mesh_asset(AUID meshAUID);

    private:
        MeshComponent* mComp;
        CUID mCUID;
    };

    /// @brief Public interface for Sprite2D components.
    class Sprite2D
    {
    public:
        Sprite2D() = delete;
        Sprite2D(Sprite2DComponent* comp);

        inline operator bool() const noexcept { return mComp && mComp->draw; }

        void set_texture_2d_asset(AUID textureAUID);
        inline uint32_t get_z_depth() { return mComp->draw.get_z_depth(); }
        inline void set_z_depth(uint32_t zDepth) { mComp->draw.set_z_depth(zDepth); }
        inline Rect get_rect() { return mComp->draw.get_rect(); }
        inline void set_rect(const Rect& rect) { mComp->draw.set_rect(rect); }

    private:
        Sprite2DComponent* mComp;
        CUID mCUID;
    };
};

} // namespace LD