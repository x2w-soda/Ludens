#pragma once

#include <Ludens/AudioServer/AudioServer.h>
#include <Ludens/RenderServer/RenderServer.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/System/FileSystem.h>
#include <LudensEditor/EditorContext/EditorAction.h>
#include <LudensEditor/EditorContext/EditorCallback.h>
#include <LudensEditor/EditorContext/EditorContextEvent.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct ComponentBase;
struct Transform;

struct EditorContextInfo
{
    AudioServer audioServer;   /// audio server handle
    RenderServer renderServer; /// render server handle
    FS::Path iconAtlasPath;    /// path to icon atlas
};

/// @brief Shared context among editor windows. Keeps track of
///        the Scene being edited. All edits to the current Scene
///        goes through this class.
struct EditorContext : Handle<struct EditorContextObj>
{
    /// @brief Create the editor context.
    static EditorContext create(const EditorContextInfo& info);

    /// @brief Destroy the editor context.
    static void destroy(EditorContext ctx);

    /// @brief Callback to inform the render server the transforms of RUIDs
    static Mat4 render_server_transform_callback(RUID ruid, void* user);

    /// @brief Redo most recent undo.
    void action_redo();

    /// @brief Undo previous edit.
    void action_undo();

    /// @brief Create new empty scene.
    void action_new_scene(const FS::Path& sceneSchemaPath);

    /// @brief Open existing scene.
    void action_open_scene(const FS::Path& sceneSchemaPath);

    /// @brief Save the current scene schema to disk.
    void action_save_scene();

    /// @brief Add script to component.
    void action_add_component_script(CUID compID, AUID scriptAssetID);

    /// @brief Complete all editor actions in queue.
    void poll_actions();

    /// @brief Get directory of current project.
    FS::Path get_project_directory();

    /// @brief Get path to current scene schema.
    FS::Path get_scene_schema_path();

    /// @brief Get editor global settings
    EditorSettings get_settings();

    /// @brief Get asset manager handle.
    AssetManager get_asset_manager();

    /// @brief Get icon atlas for editor.
    RImage get_editor_icon_atlas();

    /// @brief Get editor theme.
    inline EditorTheme get_theme() { return get_settings().get_theme(); }

    /// @brief Get scene handle.
    Scene get_scene();

    /// @brief Get camera to render scene with.
    Camera get_scene_camera();

    /// @brief Add an observer of the editor context
    void add_observer(EditorContextEventFn fn, void* user);

    /// @brief Editor context frame update, if the scene is playing, this calls the scene update.
    /// @param sceneExtent Screen size containing the scene.
    /// @param delta Delta time in seconds.
    void update(const Vec2& sceneExtent, float delta);

    ///@brief Load a Project to edit
    ///@warning Experimental
    ///@note Triggers EDITOR_CONTEXT_EVENT_PROJECT_LOAD for observers
    void load_project(const FS::Path& projectSchemaPath);

    /// @brief Load a Scene from the current Project
    /// @warning Experimental
    /// @note Triggers EDITOR_CONTEXT_EVENT_SCENE_LOAD for observers.
    void load_project_scene(const FS::Path& sceneSchemaPath);

    /// @brief Begin playing scene in the editor.
    void play_scene();

    /// @brief Stop playing scene in the editor.
    void stop_scene();

    /// @brief Whether or not the scene simulation is playing in editor.
    bool is_playing();

    /// @brief Get root data components in scene
    void get_scene_roots(std::vector<CUID>& roots);

    /// @brief Get component base members.
    const ComponentBase* get_component_base(CUID comp);

    /// @brief Get the C string name of a component
    const char* get_component_name(CUID comp);

    /// @brief Get component script slot.
    const ComponentScriptSlot* get_component_script_slot(CUID compID);

    /// @brief Assign a component in scene to be selected.
    /// @note Triggers EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION for observers.
    void set_selected_component(CUID comp);

    /// @brief Get the currently selected component in scene
    CUID get_selected_component();

    /// @brief Get component in current Scene
    void* get_component(CUID compID, ComponentType& type);

    /// @brief Get the data component associated with some RUID in scene
    CUID get_ruid_component(RUID ruid);

    /// @brief get the RUID associated with the selected object in scene
    RUID get_selected_component_ruid();

    /// @brief Get the local transform associated with the selected object in scene.
    bool get_selected_component_transform(Transform& transform);

    /// @brief Set local transform of a component.
    bool set_component_transform(CUID compID, const Transform& transform);

    /// @brief Get component world matrix.
    bool get_component_transform_mat4(CUID compID, Mat4& worldMat4);
};

} // namespace LD