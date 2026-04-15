#include <Ludens/Header/Assert.h>
#include <Ludens/Scene/ComponentViews.h>
#include <LudensEditor/EditorContext/EditCommand.h>
#include <LudensEditor/EditorContext/EditorContext.h>

#include "EditorContextObj.h"

namespace LD {

static EditCommand* rename_component_command_create() { return heap_new<RenameComponentCommand>(MEMORY_USAGE_MISC); }
static void rename_component_command_destroy(EditCommand* baseCmd) { heap_delete<RenameComponentCommand>((RenameComponentCommand*)baseCmd); }
static void rename_component_command_redo(EditCommand* baseCmd);
static void rename_component_command_undo(EditCommand* baseCmd);
static EditCommand* add_component_command_create() { return heap_new<AddComponentCommand>(MEMORY_USAGE_MISC); }
static void add_component_command_destroy(EditCommand* baseCmd) { heap_delete<AddComponentCommand>((AddComponentCommand*)baseCmd); }
static void add_component_command_redo(EditCommand* baseCmd);
static void add_component_command_undo(EditCommand* baseCmd);
static EditCommand* add_component_script_command_create() { return heap_new<AddComponentScriptCommand>(MEMORY_USAGE_MISC); }
static void add_component_script_command_destroy(EditCommand* baseCmd) { heap_delete<AddComponentScriptCommand>((AddComponentScriptCommand*)baseCmd); }
static void add_component_script_command_redo(EditCommand* baseCmd);
static void add_component_script_command_undo(EditCommand* baseCmd);
static EditCommand* set_component_asset_command_create() { return heap_new<SetComponentAssetCommand>(MEMORY_USAGE_MISC); }
static void set_component_asset_command_destroy(EditCommand* baseCmd) { heap_delete<SetComponentAssetCommand>((SetComponentAssetCommand*)baseCmd); }
static void set_component_asset_command_redo(EditCommand* baseCmd);
static void set_component_asset_command_undo(EditCommand* baseCmd);
static void set_component_asset(Scene scene, SUID compSUID, AssetID assetID);
static EditCommand* set_component_transform_2d_command_create() { return heap_new<SetComponentTransform2DCommand>(MEMORY_USAGE_MISC); }
static void set_component_transform_2d_command_destroy(EditCommand* baseCmd) { heap_delete<SetComponentTransform2DCommand>((SetComponentTransform2DCommand*)baseCmd); }
static void set_component_transform_2d_command_redo(EditCommand* baseCmd);
static void set_component_transform_2d_command_undo(EditCommand* baseCmd);
static EditCommand* clone_component_subtree_command_create() { return heap_new<CloneComponentSubtreeCommand>(MEMORY_USAGE_MISC); }
static void clone_component_subtree_command_destroy(EditCommand* baseCmd) { heap_delete<CloneComponentSubtreeCommand>((CloneComponentSubtreeCommand*)baseCmd); }
static void clone_component_subtree_command_redo(EditCommand* baseCmd);
static void clone_component_subtree_command_undo(EditCommand* baseCmd);
static EditCommand* delete_component_subtree_command_create() { return heap_new<DeleteComponentSubtreeCommand>(MEMORY_USAGE_MISC); }
static void delete_component_subtree_command_destroy(EditCommand* baseCmd) { heap_delete<DeleteComponentSubtreeCommand>((DeleteComponentSubtreeCommand*)baseCmd); }
static void delete_component_subtree_command_redo(EditCommand* baseCmd);
static void delete_component_subtree_command_undo(EditCommand* baseCmd);

struct EditCommandMeta
{
    EditCommand* (*create)();
    void (*destroy)(EditCommand* baseCmd);
    void (*redo)(EditCommand* baseCmd);
    void (*undo)(EditCommand* baseCmd);
};

static EditCommandMeta sEditCommand[]{
    {&rename_component_command_create, &rename_component_command_destroy, &rename_component_command_redo, &rename_component_command_undo},
    {&add_component_command_create, &add_component_command_destroy, &add_component_command_redo, &add_component_command_undo},
    {&add_component_script_command_create, &add_component_script_command_destroy, &add_component_script_command_redo, &add_component_script_command_undo},
    {&set_component_asset_command_create, &set_component_asset_command_destroy, &set_component_asset_command_redo, &set_component_asset_command_undo},
    {&set_component_transform_2d_command_create, &set_component_transform_2d_command_destroy, &set_component_transform_2d_command_redo, &set_component_transform_2d_command_undo},
    {&clone_component_subtree_command_create, &clone_component_subtree_command_destroy, &clone_component_subtree_command_redo, &clone_component_subtree_command_undo},
    {&delete_component_subtree_command_create, &delete_component_subtree_command_destroy, &delete_component_subtree_command_redo, &delete_component_subtree_command_undo},
};

static_assert(sizeof(sEditCommand) / sizeof(*sEditCommand) == (int)EDIT_COMMAND_TYPE_ENUM_COUNT);

void EditCommand::redo(EditCommand* cmd)
{
    sEditCommand[(int)cmd->type].redo(cmd);
}

void EditCommand::undo(EditCommand* cmd)
{
    sEditCommand[(int)cmd->type].undo(cmd);
}

EditCommand* EditCommand::create(EditCommandType type, EditorContextObj* ctx)
{
    EditCommand* cmd = sEditCommand[(int)type].create();
    cmd->type = type;
    cmd->ctx = ctx;

    return cmd;
}

void EditCommand::destroy(EditCommand* cmd)
{
    sEditCommand[(int)cmd->type].destroy(cmd);
}

void RenameComponentCommand::configure(SUID compSUID, const std::string& newName)
{
    ComponentView comp = ctx->scene.get_component_by_suid(compSUID);
    LD_ASSERT(comp); // TODO: recovery

    this->compSUID = compSUID;
    this->newName = newName;
    this->oldName = comp.get_name();
}

void AddComponentCommand::configure(SUID parentSUID, ComponentType compType)
{
    LD_ASSERT(parentSUID);

    this->parentSUID = parentSUID;
    this->compType = compType;
}

static void set_component_asset(Scene scene, SUID compSUID, AssetID assetID)
{
    if (!compSUID || !assetID)
        return;

    ComponentView comp = scene.get_component_by_suid(compSUID);
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

static void rename_component_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (RenameComponentCommand*)baseCmd;
    ComponentView comp = cmd->ctx->scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp); // TODO: recovery

    comp.set_name(cmd->newName.c_str());

    EditorContext(cmd->ctx).set_selected_component(comp.cuid());
}

static void rename_component_command_undo(EditCommand* baseCmd)
{
    auto* cmd = (RenameComponentCommand*)baseCmd;
    ComponentView comp = cmd->ctx->scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp); // TODO: recovery

    comp.set_name(cmd->oldName.c_str());

    EditorContext(cmd->ctx).set_selected_component(comp.cuid());
}

static void add_component_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (AddComponentCommand*)baseCmd;
    Scene scene = cmd->ctx->scene;

    // TODO: resolve name collisions
    const char* name = get_component_type_name(cmd->compType);

    SUIDRegistry suidReg = cmd->ctx->projectCtx.suid_registry();
    ComponentView comp = scene.create_component_serial(cmd->compType, name, suidReg, cmd->parentSUID, (SUID)0);
    LD_ASSERT(comp); // TODO: recovery

    cmd->compSUID = comp.suid();
}

static void add_component_command_undo(EditCommand* baseCmd)
{
    auto* cmd = (AddComponentCommand*)baseCmd;
    Scene scene = cmd->ctx->scene;

    ComponentView comp = scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp); // TODO: recovery

    scene.destroy_component_subtree(comp.cuid());

    cmd->compSUID = 0;
}

void AddComponentScriptCommand::configure(SUID compSUID, AssetID scriptAssetID)
{
    LD_ASSERT(compSUID && scriptAssetID);

    this->compSUID = compSUID;
    this->scriptAssetID = scriptAssetID;
}

static void add_component_script_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (AddComponentScriptCommand*)baseCmd;

    ComponentView comp = cmd->ctx->scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp);

    cmd->prevScriptAssetID = comp.get_script_asset_id();
    comp.set_script_asset_id(cmd->scriptAssetID);
}

static void add_component_script_command_undo(EditCommand* baseCmd)
{
    auto* cmd = (AddComponentScriptCommand*)baseCmd;

    ComponentView comp = cmd->ctx->scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp);

    comp.set_script_asset_id(cmd->prevScriptAssetID);
}

void SetComponentAssetCommand::configure(SUID compSUID, AssetID assetID)
{
    LD_ASSERT(compSUID && assetID);
    this->compSUID = compSUID;
    this->assetID = assetID;
    prevAssetID = 0;
}

static void set_component_asset_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (SetComponentAssetCommand*)baseCmd;
    Scene scene = cmd->ctx->scene;

    ComponentView comp = scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp);

    cmd->prevAssetID = comp.get_script_asset_id();

    set_component_asset(scene, cmd->compSUID, cmd->assetID);
}

static void set_component_asset_command_undo(EditCommand* baseCmd)
{
    auto* cmd = (SetComponentAssetCommand*)baseCmd;

    set_component_asset(cmd->ctx->scene, cmd->compSUID, cmd->prevAssetID);
}

static void set_component_transform_2d_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (SetComponentTransform2DCommand*)baseCmd;
    ComponentView comp = cmd->ctx->scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp);

    (void)comp.set_transform_2d(cmd->transform);
}

static void set_component_transform_2d_command_undo(EditCommand* baseCmd)
{
    auto* cmd = (SetComponentTransform2DCommand*)baseCmd;
    ComponentView comp = cmd->ctx->scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp);

    (void)comp.set_transform_2d(cmd->prevTransform);
}

static void clone_component_subtree_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (CloneComponentSubtreeCommand*)baseCmd;
    Scene scene = cmd->ctx->scene;

    ComponentView srcComp = scene.get_component_by_path(cmd->srcPath);
    LD_ASSERT(srcComp);

    SUIDRegistry suidReg = cmd->ctx->projectCtx.suid_registry();
    ComponentView dstComp = scene.clone_component_subtree(srcComp.cuid(), suidReg);
    LD_ASSERT(dstComp);

    Transform2D transform;
    if (srcComp.get_transform_2d(transform))
    {
        transform.position += Vec2(10.0f, 10.0f);
        dstComp.set_transform_2d(transform);
    }

    cmd->dstCUID = dstComp.cuid();

    EditorContext(cmd->ctx).set_selected_component(cmd->dstCUID);
}

static void clone_component_subtree_command_undo(EditCommand* baseCmd)
{
    auto* cmd = (CloneComponentSubtreeCommand*)baseCmd;
    Scene scene = cmd->ctx->scene;

    scene.destroy_component_subtree(cmd->dstCUID);

    cmd->dstCUID = 0;
}

void CloneComponentSubtreeCommand::configure(SUID compSUID)
{
    LD_ASSERT(compSUID);

    Scene scene = ctx->scene;
    ComponentView srcComp = scene.get_component_by_suid(compSUID);
    bool ok = scene.get_component_path(srcComp, srcPath);
    LD_ASSERT(ok);
}

void DeleteComponentSubtreeCommand::configure(SUID compSUID)
{
    this->compSUID = compSUID;
}

static void delete_component_subtree_command_redo(EditCommand* baseCmd)
{
    auto* cmd = (DeleteComponentSubtreeCommand*)baseCmd;
    Scene scene = cmd->ctx->scene;

    ComponentView comp = scene.get_component_by_suid(cmd->compSUID);
    LD_ASSERT(comp);
    scene.destroy_component_subtree(comp.cuid());
}

static void delete_component_subtree_command_undo(EditCommand* baseCmd)
{
    LD_UNREACHABLE;
}

} // namespace LD