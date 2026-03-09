#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/MeshView.h>

#include "MeshComponent.h"

namespace LD {

void init_mesh_component(ComponentBase** dstData)
{
    MeshComponent* dstMesh = (MeshComponent*)dstData;
    dstMesh->draw = {};
    dstMesh->assetID = 0;
}

bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID, std::string& err)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = mesh->base;

    mesh->draw = scene->renderSystemCache.create_mesh_draw(base->cuid, meshAID);

    if (!mesh->draw)
    {
        err = "RenderSystem failed to create MeshDraw";
        return false;
    }

    mesh->assetID = meshAID;

    return true;
}

bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    MeshView dstMesh(dstData);
    MeshView srcMesh(srcData);
    LD_ASSERT(dstMesh && srcMesh);

    AssetID srcMeshAID = srcMesh.get_mesh_asset();

    return load_mesh_component(scene, (MeshComponent*)dstData, srcMeshAID, err);
}

bool unload_mesh_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    MeshComponent* mesh = (MeshComponent*)data;
    ComponentBase* base = mesh->base;

    LD_ASSERT(mesh->draw);
    scene->renderSystemCache.destroy_mesh_draw(mesh->draw);
    mesh->draw = {};

    return true;
}

MeshView::MeshView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_MESH)
    {
        mData = comp.data();
        mMesh = (MeshComponent*)mData;
    }
}

MeshView::MeshView(MeshComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mMesh = comp;
    }
}

bool MeshView::load()
{
    std::string err;

    return load_mesh_component(sScene, mMesh, (AssetID)0, err);
}

bool MeshView::set_mesh_asset(AssetID meshID)
{
    LD_ASSERT(mMesh->draw);

    MeshData meshData = sScene->renderSystemCache.get_or_create_mesh_data(meshID);

    if (mMesh->draw.set_mesh_asset(meshData))
    {
        mMesh->assetID = meshID;
        return true;
    }

    return false;
}

AssetID MeshView::get_mesh_asset()
{
    return mMesh->assetID;
}

} // namespace LD