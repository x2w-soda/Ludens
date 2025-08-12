#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/System/Memory.h>
#include <iostream>
#include <vector>

namespace LD {

struct SceneObj
{
    DataRegistry registry;
    AssetManager assetManager;
    RServer renderServer;
    std::unordered_map<RUID, Transform*> ruidTransforms; /// map RUID to its corresponding transform
    std::unordered_map<RUID, DUID> ruidToComponent;      /// map RUID to its corresponding component in the data registry
    std::vector<DUID> roots;                             /// scene root components

    /// @brief Load components into data registry from JSON document.
    void load_registry_from_json(JSONDocument jsonDoc);
};

void SceneObj::load_registry_from_json(JSONDocument jsonDoc)
{
    roots.clear();

    JSONNode root = jsonDoc.get_root();

    JSONNode objects = root.get_member("objects");
    LD_ASSERT(objects.is_array());
    int objectCount = objects.get_size();

    for (int i = 0; i < objectCount; i++)
    {
        JSONNode object = objects.get_index(i);
        LD_ASSERT(object.is_object());

        JSONNode name = object.get_member("name");
        std::string nameStr;
        if (name && name.is_string(&nameStr))
            std::cout << "loading object: " << nameStr << std::endl;

        JSONNode comp = object.get_member("MeshComponent");
        if (!comp || !comp.is_object())
            continue;

        ComponentType componentType;
        DUID meshCID = registry.create_component(COMPONENT_TYPE_MESH, nameStr.c_str());
        MeshComponent* meshC = (MeshComponent*)registry.get_component(meshCID, componentType);
        roots.push_back(meshCID);

        JSONNode member = comp.get_member("auid");
        uint32_t auid;
        if (member && member.is_u32(&auid))
        {
            MeshAsset meshA = assetManager.get_mesh_asset(auid);
            meshC->ruid = renderServer.create_mesh(*meshA.data());
            ruidTransforms[meshC->ruid] = registry.get_component_transform(meshCID);
            ruidToComponent[meshC->ruid] = meshCID;
        }

        member = comp.get_member("transform");
        if (member && member.is_object())
        {
            Transform& transform = meshC->transform;
            transform.position = Vec3(0.0f);
            transform.rotation = Vec3(0.0f);
            transform.scale = Vec3(1.0f);

            JSONNode prop = member.get_member("position");
            if (prop && prop.is_array() && prop.get_size() == 3)
            {
                prop.get_index(0).is_f32(&transform.position.x);
                prop.get_index(1).is_f32(&transform.position.y);
                prop.get_index(2).is_f32(&transform.position.z);
            }

            prop = member.get_member("rotation");
            if (prop && prop.is_array() && prop.get_size() == 3)
            {
                prop.get_index(0).is_f32(&transform.rotation.x);
                prop.get_index(1).is_f32(&transform.rotation.y);
                prop.get_index(2).is_f32(&transform.rotation.z);
            }

            prop = member.get_member("scale");
            if (prop && prop.is_array() && prop.get_size() == 3)
            {
                prop.get_index(0).is_f32(&transform.scale.x);
                prop.get_index(1).is_f32(&transform.scale.y);
                prop.get_index(2).is_f32(&transform.scale.z);
            }
        }
    }
}

// NOTE: Experimental, this is mostly for the Editor to load the scene.
//       Runtime scene loading will probably use a different path.
Scene Scene::create(const SceneInfo& info)
{
    LD_PROFILE_SCOPE;
    SceneObj* obj = heap_new<SceneObj>(MEMORY_USAGE_SCENE);
    obj->registry = DataRegistry::create();
    obj->assetManager = info.assetManager;
    obj->renderServer = info.renderServer;

    DataRegistry registry = obj->registry;
    obj->load_registry_from_json(info.jsonDoc);

    return Scene(obj);
}

void Scene::destroy(Scene scene)
{
    SceneObj* obj = scene.unwrap();

    DataRegistry::destroy(obj->registry);

    heap_delete<SceneObj>(obj);
}

void Scene::get_root_components(std::vector<DUID>& roots)
{
    roots = mObj->roots;
}

const DataComponent* Scene::get_component_base(DUID compID)
{
    return mObj->registry.get_component_base(compID);
}

void* Scene::get_component(DUID compID, ComponentType& type)
{
    return mObj->registry.get_component(compID, type);
}

RUID Scene::get_component_ruid(DUID compID)
{
    return mObj->registry.get_component_ruid(compID);
}

Transform* Scene::get_component_transform(DUID compID)
{
    // pointer stability: valid until component is destroyed
    return mObj->registry.get_component_transform(compID);
}

DUID Scene::get_ruid_component(RUID ruid)
{
    auto ite = mObj->ruidToComponent.find(ruid);

    if (ite == mObj->ruidToComponent.end())
        return 0;

    return ite->second;
}

Mat4 Scene::get_ruid_transform(RUID ruid)
{
    // TODO: optimize, this is terrible and lazy
    Transform* transform = mObj->ruidTransforms[ruid];
    const Vec3& axisR = transform->rotation;
    Mat4 R = Mat4::rotate(LD_TO_RADIANS(axisR.x), Vec3(1.0f, 0.0f, 0.0f)) * Mat4::rotate(LD_TO_RADIANS(axisR.y), Vec3(0.0f, 1.0f, 0.0f)) * Mat4::rotate(LD_TO_RADIANS(axisR.z), Vec3(0.0f, 0.0f, 1.0f));
    return Mat4::translate(transform->position) * R * Mat4::scale(transform->scale);
}

} // namespace LD