#include <Ludens/Profiler/Profiler.h>

#include "MeshComponent.h"

namespace LD {

bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = mesh->base;

    mesh->draw = scene->renderSystemCache.create_mesh_draw(base->cuid, meshAID);

    if (!mesh->draw)
        return false;

    mesh->assetID = meshAID;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::Mesh dstMesh(dstData);
    Scene::Mesh srcMesh(srcData);
    LD_ASSERT(dstMesh && srcMesh);

    AssetID srcMeshAID = srcMesh.get_mesh_asset();

    return load_mesh_component(scene, (MeshComponent*)dstData, srcMeshAID);
}

void unload_mesh_component(SceneObj* scene, ComponentBase** data)
{
    MeshComponent* mesh = (MeshComponent*)data;
    ComponentBase* base = mesh->base;

    LD_ASSERT(mesh->draw);
    scene->renderSystemCache.destroy_mesh_draw(mesh->draw);
    mesh->draw = {};

    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

Scene::Mesh::Mesh(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_MESH)
    {
        mData = comp.data();
        mMesh = (MeshComponent*)mData;
    }
}

Scene::Mesh::Mesh(MeshComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mMesh = comp;
    }
}

bool Scene::Mesh::load()
{
    return load_mesh_component(sScene, mMesh, (AssetID)0);
}

bool Scene::Mesh::set_mesh_asset(AssetID meshID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    MeshData meshData = sScene->renderSystemCache.get_or_create_mesh_data(meshID);

    if (mMesh->draw.set_mesh_asset(meshData))
    {
        mMesh->assetID = meshID;
        return true;
    }

    return false;
}

AssetID Scene::Mesh::get_mesh_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mMesh->assetID;
}

} // namespace LD