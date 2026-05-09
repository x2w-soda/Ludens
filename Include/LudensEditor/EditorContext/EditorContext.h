#pragma once

#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/KeyValue.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSettings.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Serial/SUIDTable.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UILayout.h>
#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>
#include <LudensBuilder/DocumentBuilder/Document.h>
#include <LudensEditor/EditorContext/EditorContextDef.h>
#include <LudensEditor/EditorContext/EditorEvent.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct ComponentBase;
struct Transform;
struct TransformEx;
struct ProjectScanResult;
struct AssetImportInfo;
struct AssetCreateInfo;

struct EditorContextInfo
{
    AudioSystem audioSystem;   /// audio system handle
    RenderSystem renderSystem; /// render system handle
    FS::Path iconAtlasPath;    /// path to icon atlas
    FontAtlas defaultFontAtlas;
    FontAtlas monoFontAtlas;
    RImage defaultFontAtlasImage;
    RImage monoFontAtlasImage;
    size_t projectScanResultCount = 0;
    const ProjectScanResult* projectScanResults = nullptr;
};

struct EditorContextAssetInterface : Handle<struct EditorContextObj>
{
    AssetImportInfo* alloc_asset_import_info(AssetType type);
    void free_asset_import_info(AssetImportInfo* info);
    AssetCreateInfo* alloc_asset_create_info(AssetCreateType type);
    void free_asset_create_info(AssetCreateInfo* info);

    /// @brief Blocking call to create and import Asset into project.
    /// @return Asset handle upon success.
    Asset create_asset(AssetCreateInfo* info, const String& importDstPath, String& err);
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

    /// @brief Callback to inform the render system the model matrix of RUIDs
    static bool render_system_mat4_callback(RUID ruid, Mat4& mat4, void* user);

    /// @brief Callback to handle file drops into the editor window.
    static void drop_file_callback(size_t fileCount, const FS::Path* files, void* user);

    /// @brief Add an EditorEvent to the event queue for deferred processing.
    ///        This is non-blocking and returns the event that is allocated by EditorContext.
    EditorEvent* enqueue_event(EditorEventType type);

    /// @brief Blocks until all events are processed.
    void poll_events();

    inline EditorContextAssetInterface asset_interface() const { return {mObj}; }

    /// @brief Check if the Project in RAM is different from the one on disk.
    bool is_project_dirty();

    /// @brief Get discovered projects.
    Vector<EditorProjectEntry> get_project_entries();

    /// @brief Get all scene entries in current project.
    Vector<SUIDEntry> get_project_scene_entries();

    /// @brief Get directory of current project.
    FS::Path get_project_directory();

    /// @brief Get path to current scene schema.
    FS::Path get_scene_schema_path();

    /// @brief Get editor global settings
    EditorSettings settings();

    /// @brief Get project handle, nullable.
    Project get_project();

    /// @brief Get project settings for loaded project.
    ProjectSettings get_project_settings();

    /// @brief Get serial ID registry for loaded project.
    SUIDRegistry get_suid_registry();

    /// @brief Get icon atlas for editor.
    RImage get_editor_icon_atlas();

    /// @brief Get editor default font.
    UIFont get_font_default();

    /// @brief Get editor monospace font.
    UIFont get_font_mono();

    /// @brief Get editor theme.
    inline EditorTheme get_theme() { return settings().get_theme(); }

    /// @brief Get scene handle.
    Scene get_scene();
    Vector<RenderSystemScreenPass::Region> get_scene_screen_regions();
    Camera get_scene_camera();
    Vec4 get_scene_clear_color();

    /// @brief Add an observer of the editor context
    void add_observer(EditorEventFn fn, void* user);

    void input_key_value(KeyValue keyVal);

    /// @brief Editor context frame update, if the scene is playing, this calls the scene update.
    /// @param sceneExtent Screen size containing the scene.
    /// @param delta Delta time in seconds.
    void update(const Vec2& sceneExtent, float delta);

    /// @brief Begin playing scene in the editor.
    /// @return True upon successful startup.
    bool play_scene();

    /// @brief Stop playing scene in the editor.
    /// @note Reselects the Component that was selected before the play session,
    ///       emits EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION for observers.
    void stop_scene();

    /// @brief Whether or not the scene simulation is playing in editor.
    bool is_playing();

    /// @brief Get root data components in scene
    void get_scene_roots(Vector<ComponentView>& roots);

    /// @brief Get the C string name of a component
    const char* get_component_name(CUID compCUID);

    /// @brief Assign a component in scene to be selected.
    /// @note Emits EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION for observers.
    void set_selected_component(CUID compCUID);

    /// @brief Get the currently selected component in scene
    CUID get_selected_component();

    ComponentView get_selected_component_view();

    /// @brief Get component interface.
    ComponentView get_component(CUID compCUID);

    /// @brief Get component by SUID, only "editor components" have a serial ID.
    ComponentView get_component_by_suid(SUID compSUID);

    /// @brief Get component associated with RUID.
    ComponentView get_component_by_ruid(RUID ruid);

    /// @brief get the RUID associated with the selected object in scene
    RUID get_selected_component_ruid();

    /// @brief Get the local transform associated with the selected object in scene.
    bool get_selected_component_transform(TransformEx& transform);

    /// @brief Get document from its URI path. Could fail and return null handle.
    Document get_document(const char* uriPath);
};

} // namespace LD