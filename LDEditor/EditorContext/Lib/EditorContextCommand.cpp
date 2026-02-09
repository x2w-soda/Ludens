#include <Ludens/Header/Assert.h>

#include "EditorContextCommand.h"

namespace LD {

AddComponentCommand::AddComponentCommand(Scene scene, SUID parentSUID, ComponentType compType)
    : mScene(scene), mParentSUID(parentSUID), mCompType(compType)
{
    LD_ASSERT(mScene && mParentSUID);
}

void AddComponentCommand::redo()
{
    // TODO: resolve name collisions
    const char* name = get_component_type_name(mCompType);

    Scene::Component comp = mScene.create_component_serial(mCompType, name, mParentSUID);
    LD_ASSERT(comp); // TODO: recovery

    mCompSUID = comp.suid();
}

void AddComponentCommand::undo()
{
    Scene::Component comp = mScene.get_component_by_suid(mCompSUID);
    LD_ASSERT(comp); // TODO: recovery

    mScene.destroy_component(comp.cuid());

    mCompSUID = 0;
}

AddComponentScriptCommand::AddComponentScriptCommand(Scene scene, SUID compSUID, AssetID scriptAssetID)
    : mScene(scene), mCompSUID(compSUID), mScriptAssetID(scriptAssetID)
{
    LD_ASSERT(mScene && mCompSUID && mScriptAssetID);

    Scene::Component comp = mScene.get_component_by_suid(mCompSUID);

    if (comp)
        mPrevScriptAssetID = comp.get_script_asset_id();
}

void AddComponentScriptCommand::redo()
{
    Scene::Component comp = mScene.get_component_by_suid(mCompSUID);

    if (comp)
        comp.set_script_asset_id(mScriptAssetID);
}

void AddComponentScriptCommand::undo()
{
    Scene::Component comp = mScene.get_component_by_suid(mCompSUID);

    if (comp)
        comp.set_script_asset_id(mPrevScriptAssetID);
}

SetComponentAssetCommand::SetComponentAssetCommand(Scene scene, SUID compSUID, AssetID assetID)
    : mScene(scene), mCompSUID(compSUID), mAssetID(assetID), mPrevAssetID(/* TODO: */ 0)
{
    LD_ASSERT(mScene && mCompSUID && mAssetID);
}

void SetComponentAssetCommand::redo()
{
    set_component_asset(mCompSUID, mAssetID);
}

void SetComponentAssetCommand::undo()
{
    set_component_asset(mCompSUID, mPrevAssetID);
}

void SetComponentAssetCommand::set_component_asset(SUID compSUID, AssetID assetID)
{
    if (!compSUID || !assetID)
        return;

    Scene::Component comp = mScene.get_component_by_suid(compSUID);
    if (!comp)
        return;

    switch (comp.type())
    {
    case COMPONENT_TYPE_AUDIO_SOURCE:
    {
        Scene::AudioSource source(comp);
        LD_ASSERT(source);
        source.set_clip_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_MESH:
    {
        Scene::Mesh mesh(comp);
        LD_ASSERT(mesh);
        mesh.set_mesh_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_SPRITE_2D:
    {
        Scene::Sprite2D sprite((Sprite2DComponent*)comp.data());
        sprite.set_texture_2d_asset(assetID);
        break;
    }
    default:
        break;
    }
}

} // namespace LD