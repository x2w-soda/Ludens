#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Scene/ComponentViews.h>
#include <Ludens/System/Process.h>
#include <LudensEditor/EditorContext/EditorContextUtil.h>
#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIComponent.h>
#include <LudensEditor/EditorWidget/EUIProp.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

/*
static void eui_inspect_audio_source_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_transform_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_transform_2d_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_camera_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_camera_2d_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_mesh_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_sprite_2d_component(EUIComponentStorage* storage, ComponentView comp);

static void (*sEUIInspectFnTable[])(EUIComponentStorage* storage, ComponentView comp) = {
nullptr,
&eui_inspect_audio_source_component,
&eui_inspect_transform_component,
&eui_inspect_transform_2d_component,
&eui_inspect_camera_component,
&eui_inspect_camera_2d_component,
&eui_inspect_mesh_component,
&eui_inspect_sprite_2d_component,
nullptr,
};

static_assert(sizeof(sEUIInspectFnTable) / sizeof(*sEUIInspectFnTable) == COMPONENT_TYPE_ENUM_COUNT);

void eui_inspect_audio_source_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE);

eui_component_audio_source(&storage->as.audioSource, comp);
}

void eui_inspect_transform_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM);

LD_UNREACHABLE;
}

void eui_inspect_transform_2d_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM_2D);

eui_component_transform_2d(&storage->as.transform2D, comp);
}

void eui_inspect_camera_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA);

LD_UNREACHABLE;
}

void eui_inspect_camera_2d_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA_2D);

eui_component_camera_2d(&storage->as.camera2D, comp);
}

static void eui_inspect_mesh_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_MESH);

LD_UNREACHABLE;
}

void eui_inspect_sprite_2d_component(EUIComponentStorage* storage, ComponentView comp)
{
LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_SPRITE_2D);

eui_component_sprite_2d(&storage->as.sprite2D, comp);
}
*/

void eui_inspect_component(EditorContext& ctx, ComponentView comp)
{
    LD_ASSERT(comp);

    /*
    if (type != storage->type)
    {
        storage->cleanup();
        storage->startup(type);
    }

    if (sEUIInspectFnTable[(int)type])
        sEUIInspectFnTable[(int)type](storage, comp);
    */

    Vector<PropertyDelta> delta = eui_component_property_table(comp);
    EditorContextUtil::set_component_props(ctx, comp.suid(), delta);
}

void eui_inspect_component_script(EditorContext& ctx, ComponentView comp)
{
    LD_ASSERT(comp);

    AssetID scriptID = comp.get_script_asset_id();
    AssetManager AM = AssetManager::get();
    AssetRegistry AR = AM.get_asset_registry(); // TODO: nullable
    AssetEntry entry = {};

    if (AR)
        entry = AR.get_entry(scriptID);

    if (eui_asset_slot(scriptID, ASSET_TYPE_LUA_SCRIPT))
        EditorContextUtil::request_component_script(ctx, comp.suid());

    UIButtonData* btn = (UIButtonData*)ui_push_button(nullptr, "edit").get_data();
    btn->isEnabled = (bool)scriptID;
    if (ui_button_is_pressed() && entry)
    {
        FS::Path sourceFilePath = ctx.get_project().get_storage_dir_abs_path() / scriptID.to_string() / entry.get_file_path("source");
        sourceFilePath = FS::absolute(sourceFilePath);
        if (FS::exists(sourceFilePath))
            shell_open(sourceFilePath);
    }
    ui_pop();
}

} // namespace LD