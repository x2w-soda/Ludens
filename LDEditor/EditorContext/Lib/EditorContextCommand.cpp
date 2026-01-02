#include "EditorContextCommand.h"
#include <Ludens/Header/Assert.h>

namespace LD {

AddComponentScriptCommand::AddComponentScriptCommand(Scene scene, CUID compID, AUID scriptAssetID)
    : mScene(scene), mCompID(compID), mScriptAssetID(scriptAssetID), mPrevScriptAssetID(0)
{
    LD_ASSERT(mScene && mCompID && mScriptAssetID);

    ComponentScriptSlot* scriptSlot = mScene.get_component_script_slot(mCompID);
    if (scriptSlot)
        mPrevScriptAssetID = scriptSlot->assetID;
}

void AddComponentScriptCommand::redo()
{
    mScene.create_component_script_slot(mCompID, mScriptAssetID);
}

void AddComponentScriptCommand::undo()
{
    // update Scene
    if (mPrevScriptAssetID != 0)
    {
        ComponentScriptSlot* slot = mScene.get_component_script_slot(mCompID);
        LD_ASSERT(slot);
        slot->assetID = mPrevScriptAssetID;
    }
    else
    {
        mScene.destroy_component_script_slot(mCompID);
    }
}

SetComponentAssetCommand::SetComponentAssetCommand(Scene scene, CUID compID, AUID assetID)
    : mScene(scene), mCompID(compID), mAssetID(assetID), mPrevAssetID(/* TODO: */ 0)
{
    LD_ASSERT(mScene && mCompID && mAssetID);
}

void SetComponentAssetCommand::redo()
{
    set_component_asset(mCompID, mAssetID);
}

void SetComponentAssetCommand::undo()
{
    set_component_asset(mCompID, mPrevAssetID);
}

void SetComponentAssetCommand::set_component_asset(CUID compID, AUID assetID)
{
    if (!compID || !assetID)
        return;

    ComponentType compType;
    void* comp = mScene.get_component(compID, &compType);

    if (!comp)
        return;

    switch (compType)
    {
    case COMPONENT_TYPE_AUDIO_SOURCE: {
        Scene::IAudioSource source((AudioSourceComponent*)comp);
        source.set_clip_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_MESH: {
        Scene::IMesh mesh(compID);
        mesh.set_mesh_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_SPRITE_2D: {
        Scene::ISprite2D sprite((Sprite2DComponent*)comp);
        sprite.set_texture_2d_asset(assetID);
        break;
    }
    default:
        break;
    }
}

} // namespace LD