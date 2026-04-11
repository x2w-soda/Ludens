#pragma once

#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/Scene/ComponentView.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UITheme.h>

#include <functional>

namespace LD {

/// @brief Get static C string of log channel used by lua scripts.
const char* get_lua_script_log_channel_name();
const char* get_scene_log_channel_name();

using SceneLoadFn = std::function<bool(struct SceneObj*)>;

/// @brief Render system interface on behalf of the Scene.
///        Main thread only.
struct SceneRenderSystem : Handle<struct SceneObj>
{
    void configure_screen_layers(size_t count, SUID* ids, std::string* names);
};

/// @brief Scene creation info, connects to external asset manager and subsystems.
struct SceneInfo
{
    AudioSystem audioSystem = {};
    RenderSystem renderSystem = {};
    UIFont uiFont = {};
    UITheme uiTheme = {};
};

/// @brief Scene singleton, main thread only. A scene is a hierarchy of components driven by scripts.
struct Scene : Handle<struct SceneObj>
{
public:
    /// @brief Create empty scene with no components.
    static Scene create(const SceneInfo& sceneI);

    /// @brief Destroy scene.
    static void destroy();

    /// @brief Get scene singleton handle.
    static Scene get();

    /// @brief In-place reset to initial state after creation.
    void reset();

    /// @brief Load the scene. Creates resources from assets and subsystems.
    void load(const SceneLoadFn& loadFn);

    /// @brief Unload the scene. Destroys resouorces.
    void unload();

    /// @brief Backup the current loaded scene. The next Startup/Cleanup session does not mutate the backup scene.
    ///        Intended for play-in-editor, runtime will skip this.
    void backup();

    /// @brief Startup the scene for simulation. This attaches scripts to their components.
    /// @return True if the Scene is running.
    bool startup();

    /// @brief Cleanup the scene simulation. This detaches scripts from their components.
    void cleanup();

    /// @brief Update the scene with delta time.
    /// @param extent Screen size this frame.
    /// @param delta Delta time in seconds.
    void update(const Vec2& extent, float delta);

    /// @brief Non-blocking request to begin a transition to another scene.
    bool request_transition(const SceneLoadFn& loadFn);

    /// @brief Render screen UI contents.
    void render_screen_ui(ScreenRenderComponent renderer);

    /// @brief Pass input to screen UI.
    void input_screen_ui(const WindowEvent* event);

    /// @brief Get 3D camera to render Scene world contents.
    Camera get_camera();

    /// @brief In practice each Camera2DComponent will have its own viewport region.
    void get_screen_regions(Vector<Viewport>& outViewports, Vector<Rect>& outWorldAABBs);

    /// @brief Get scene render system interface
    SceneRenderSystem render_system();

    /// @brief Try create a component.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param parentCUID Parent component ID, or zero if creating a root component.
    /// @return Component interface of the newly created component on success.
    ComponentView create_component(ComponentType type, const char* name, CUID parentCUID);

    /// @brief Try create a component with serial ID.
    /// @param type Component type.
    /// @param name Component identifier.
    /// @param suidRegistry Used to validate or generate a new serial ID.
    /// @param parentSUID Parent serial ID, or zero if creating a root component.
    /// @param hintSUID If valid, the known serial ID. Otherwise the SUID registry allocates a new one.
    /// @return Component interface of the newly created component on success.
    ComponentView create_component_serial(ComponentType type, const char* name, SUIDRegistry suidRegistry, SUID parentSUID, SUID hintSUID);

    /// @brief Destroy a component subtree.
    void destroy_component_subtree(CUID compID);

    /// @brief Reparent a component subtree.
    void reparent_component_subtree(CUID compID, CUID parentID);

    /// @brief Try clone a component subtree.
    /// @param rootID The subtree root component to clone.
    /// @return Component view of the root component of the cloned subtree on success.
    ComponentView clone_component_subtree(CUID rootID, SUIDRegistry suidRegistry);

    /// @brief Get interfaces for root components in Scene.
    void get_root_components(Vector<ComponentView>& roots);

    /// @brief Get data component from ID.
    ComponentView get_component(CUID compID);

    /// @brief Get components by type.
    Vector<ComponentView> get_components(ComponentType type);

    /// @brief Get data component from ID and expected type, fails upon type mismatch.
    inline ComponentView get_component(CUID compID, ComponentType expectedType)
    {
        ComponentView comp = get_component(compID);
        if (!comp || comp.type() != expectedType)
            return {};

        return comp;
    }

    /// @brief Get data component from serial ID.
    ComponentView get_component_by_suid(SUID compSUID);

    /// @brief Get component from sibling index path.
    /// @note Slower code path intended for editor.
    ComponentView get_component_by_path(const Vector<int>& path);

    /// @brief Get data component from serial ID and expected type, fails upon type mismatch.
    inline ComponentView get_component_by_suid(SUID compSUID, ComponentType expectedType)
    {
        ComponentView comp = get_component_by_suid(compSUID);
        if (!comp || comp.type() != expectedType)
            return {};

        return comp;
    }

    /// @brief Pick a 2D component by 2D world coordinates.
    ComponentView get_2d_component_by_position(const Vec2& worldPos);

    /// @brief Lookup the data component from draw call ID
    ComponentView get_ruid_component(RUID ruid);

    /// @brief Supplies the Mat4 model matrix for a draw call
    bool get_ruid_world_mat4(RUID ruid, Mat4& mat4);

    /// @brief Get path of sibling indices.
    /// @note Slow code path intended for editor.
    bool get_component_path(ComponentView comp, Vector<int>& path);

    /// @brief Force invalidate all transforms and screen UI.
    ///        Intended for Editor to invalidate without updating the Scene.
    void invalidate();

    /// @brief Debug print component hierarchy.
    std::string print_hierarchy();
};

} // namespace LD
