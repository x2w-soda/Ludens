#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/UI/UITheme.h>

#include <functional>

namespace LD {

struct AudioSourceComponent;
struct MeshComponent;
struct Sprite2DComponent;
struct ScreenUIComponent;
struct CameraComponent;

/// @brief Get static C string of log channel used by lua scripts.
const char* get_lua_script_log_channel_name();

/// @brief Scene creation info, connects to external asset manager and subsystems.
struct SceneInfo
{
    AssetManager assetManager;
    AudioSystem audioSystem;
    RenderSystem renderSystem;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    UITheme uiTheme;
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
    void load(const std::function<bool(Scene)>& loader);

    /// @brief Unload the scene. Destroys resouorces.
    void unload();

    /// @brief Backup the current loaded scene. The next Startup/Cleanup session does not mutate the backup scene.
    ///        Intended for play-in-editor, runtime will skip this.
    void backup();

    /// @brief Startup the scene for simulation. This attaches scripts to their components.
    void startup();

    /// @brief Cleanup the scene simulation. This detaches scripts from their components.
    void cleanup();

    /// @brief Update the scene with delta time.
    /// @param screenExtent Screen size this frame.
    /// @param delta Delta time in seconds.
    void update(const Vec2& screenExtent, float delta);

    /// @brief Get camera to render the Scene with.
    Camera get_camera();

    /// @brief Public interface for all components.
    class Component
    {
    public:
        Component() = default;
        Component(ComponentBase** data)
            : mData(data) {}

        /// @brief Check for interface validity before calling any methods.
        inline operator bool() const noexcept { return mData != nullptr; }

        inline ComponentBase* base() { return *mData; }
        inline ComponentBase** data() { return mData; }
        ComponentType type();
        CUID cuid();
        SUID suid();
        RUID ruid();

        const char* get_name();
        AssetID get_script_asset_id();
        void set_script_asset_id(AssetID assetID);
        void get_children(Vector<Component>& children);
        Component get_parent();

        bool get_transform(TransformEx& transform);
        bool set_transform(const TransformEx& transform);
        bool get_transform_2d(Transform2D& transform);
        bool set_transform_2d(const Transform2D& transform);

        bool get_world_mat4(Mat4& worldMat4);

    protected:
        ComponentBase** mData = nullptr;
    };

    /// @brief Try create a component.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param parentCUID Parent component ID, or zero if creating a root component.
    /// @return Component interface of the newly created component on success.
    Component create_component(ComponentType type, const char* name, CUID parentCUID);

    /// @brief Try create a component with serial ID.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param parentSUID Parent serial ID, or zero if creating a root component.
    /// @param hintSUID Serial ID to create with, or zero if requesting a new serial ID.
    /// @return Component interface of the newly created component on success.
    Component create_component_serial(ComponentType type, const char* name, SUID parentSUID, SUID hintSUID = 0);

    /// @brief Destroy a component.
    void destroy_component(CUID compID);

    /// @brief Reparent a component
    void reparent(CUID compID, CUID parentID);

    /// @brief Get interfaces for root components in Scene.
    void get_root_components(Vector<Component>& roots);

    /// @brief Get data component from ID.
    Component get_component(CUID compID);

    /// @brief Get data component from ID and expected type, fails upon type mismatch.
    inline Component get_component(CUID compID, ComponentType expectedType)
    {
        Component comp = get_component(compID);
        if (!comp || comp.type() != expectedType)
            return {};

        return comp;
    }

    /// @brief Get data component from serial ID.
    Component get_component_by_suid(SUID compSUID);

    /// @brief Get data component from serial ID and expected type, fails upon type mismatch.
    inline Component get_component_by_suid(SUID compSUID, ComponentType expectedType)
    {
        Component comp = get_component_by_suid(compSUID);
        if (!comp || comp.type() != expectedType)
            return {};

        return comp;
    }

    /// @brief Lookup the data component from draw call ID
    Component get_ruid_component(RUID ruid);

    /// @brief Supplies the Mat4 model matrix for a draw call
    bool get_ruid_world_mat4(RUID ruid, Mat4& mat4);

    /// @brief Public interface for audio source components.
    class AudioSource : public Component
    {
    public:
        AudioSource() = delete;
        AudioSource(Component comp);
        AudioSource(AudioSourceComponent* comp);

        bool load(AssetID clipAsset, float pan, float volumeLinear);

        void play();
        void pause();
        void resume();

        bool set_clip_asset(AssetID clipID);
        AssetID get_clip_asset();

        float get_volume_linear();
        bool set_volume_linear(float volume);
        float get_pan();
        bool set_pan(float pan);

    private:
        AudioSourceComponent* mAudioSource = nullptr;
    };

    /// @brief Public interface for camera components.
    class Camera : public Component
    {
    public:
        Camera() = delete;
        Camera(Component comp);
        Camera(CameraComponent* comp);

        bool load_perspective(const CameraPerspectiveInfo& info);
        bool load_orthographic(const CameraOrthographicInfo& info);

        bool is_main_camera();
        bool is_perspective();
        bool get_perspective_info(CameraPerspectiveInfo& outInfo);
        bool get_orthographic_info(CameraOrthographicInfo& outInfo);
        void set_perspective(const CameraPerspectiveInfo& info);
        void set_orthographic(const CameraOrthographicInfo& info);

    private:
        CameraComponent* mCamera = nullptr;
    };

    /// @brief Public interface for mesh components.
    class Mesh : public Component
    {
    public:
        Mesh() = delete;
        Mesh(Component comp);
        Mesh(MeshComponent* comp);

        bool load();

        bool set_mesh_asset(AssetID meshID);
        AssetID get_mesh_asset();

    private:
        MeshComponent* mMesh = nullptr;
    };

    /// @brief Public interface for Sprite2D components.
    class Sprite2D : public Component
    {
    public:
        Sprite2D() = delete;
        Sprite2D(Component comp);
        Sprite2D(Sprite2DComponent* comp);

        bool load(SUID screenLayerSUID, AssetID textureID);

        bool set_texture_2d_asset(AssetID textureID);
        AssetID get_texture_2d_asset();
        uint32_t get_z_depth();
        void set_z_depth(uint32_t zDepth);
        Vec2 get_pivot();
        void set_pivot(const Vec2& pivot);
        Rect get_region();
        void set_region(const Rect& region);
        RUID get_screen_layer_ruid();
        SUID get_screen_layer_suid();

    private:
        Sprite2DComponent* mSprite = nullptr;
    };

    /// @brief Public interface for ScreenUI components.
    class ScreenUI : public Component
    {
    public:
        ScreenUI() = delete;
        ScreenUI(Component comp);
        ScreenUI(ScreenUIComponent* comp);

        bool load(AssetID uiTemplateID);

        bool set_ui_template_asset(AssetID uiTemplateID);
        AssetID get_ui_template_asset();

    private:
        ScreenUIComponent* mUI = nullptr;
    };
};

} // namespace LD