#include <Ludens/Header/Assert.h>
#include <Ludens/Scene/ComponentViews.h>
#include <LudensEditor/EditorContext/EditorContext.h>

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

    ComponentView comp = mScene.create_component_serial(mCompType, name, mParentSUID, 0);
    LD_ASSERT(comp); // TODO: recovery

    mCompSUID = comp.suid();
}

void AddComponentCommand::undo()
{
    ComponentView comp = mScene.get_component_by_suid(mCompSUID);
    LD_ASSERT(comp); // TODO: recovery

    mScene.destroy_component_subtree(comp.cuid());

    mCompSUID = 0;
}

AddComponentScriptCommand::AddComponentScriptCommand(Scene scene, SUID compSUID, AssetID scriptAssetID)
    : mScene(scene), mCompSUID(compSUID), mScriptAssetID(scriptAssetID)
{
    LD_ASSERT(mScene && mCompSUID && mScriptAssetID);

    ComponentView comp = mScene.get_component_by_suid(mCompSUID);

    if (comp)
        mPrevScriptAssetID = comp.get_script_asset_id();
}

void AddComponentScriptCommand::redo()
{
    ComponentView comp = mScene.get_component_by_suid(mCompSUID);

    if (comp)
        comp.set_script_asset_id(mScriptAssetID);
}

void AddComponentScriptCommand::undo()
{
    ComponentView comp = mScene.get_component_by_suid(mCompSUID);

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

    ComponentView comp = mScene.get_component_by_suid(compSUID);
    if (!comp)
        return;

    switch (comp.type())
    {
    case COMPONENT_TYPE_AUDIO_SOURCE:
    {
        AudioSourceView source(comp);
        LD_ASSERT(source);
        source.set_clip_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_MESH:
    {
        MeshView mesh(comp);
        LD_ASSERT(mesh);
        mesh.set_mesh_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_SPRITE_2D:
    {
        Sprite2DView sprite((Sprite2DComponent*)comp.data());
        sprite.set_texture_2d_asset(assetID);
        break;
    }
    default:
        break;
    }
}

CloneComponentSubtreeCommand::CloneComponentSubtreeCommand(EditorContextObj* ctx, SUID compSUID)
    : mCtx(ctx), mDstCUID(0)
{
    LD_ASSERT(mCtx && compSUID);

    Scene scene = EditorContext(ctx).get_scene();
    ComponentView srcComp = scene.get_component_by_suid(compSUID);
    bool ok = scene.get_component_path(srcComp, mSrcPath);
    LD_ASSERT(ok);
}

void CloneComponentSubtreeCommand::redo()
{
    EditorContext ctx(mCtx);
    Scene scene = ctx.get_scene();

    ComponentView srcComp = scene.get_component_by_path(mSrcPath);
    LD_ASSERT(srcComp);

    ComponentView dstComp = scene.clone_component_subtree(srcComp.cuid());
    LD_ASSERT(dstComp);

    Transform2D transform;
    if (srcComp.get_transform_2d(transform))
    {
        transform.position += Vec2(10.0f, 10.0f);
        dstComp.set_transform_2d(transform);
    }

    mDstCUID = dstComp.cuid();

    ctx.set_selected_component(mDstCUID);
}

void CloneComponentSubtreeCommand::undo()
{
    EditorContext ctx(mCtx);
    Scene scene = ctx.get_scene();

    scene.destroy_component_subtree(mDstCUID);

    mDstCUID = 0;
}

DeleteComponentSubtreeCommand::DeleteComponentSubtreeCommand(EditorContextObj* ctx, SUID compSUID)
    : mCtx(ctx), mSUID(compSUID)
{
}

void DeleteComponentSubtreeCommand::redo()
{
    EditorContext ctx(mCtx);
    Scene scene = ctx.get_scene();

    ComponentView comp = scene.get_component_by_suid(mSUID);
    LD_ASSERT(comp);
    scene.destroy_component_subtree(comp.cuid());
}

void DeleteComponentSubtreeCommand::undo()
{
    LD_UNREACHABLE;
}

} // namespace LD