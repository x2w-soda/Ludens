#include <Ludens/DSA/Queue.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/Scene/SceneCommand.h>

#define SCENE_COMMAND_ALIGNMENT 16

namespace LD {

struct SceneCommandMeta
{
    size_t size;
    void (*ctor)(void* mem);
    void (*dtor)(void* mem);
    void (*apply)(SceneCommand* cmd, Scene scene);
    const char* name;
};

static void load_scene_cmd_ctor(void* mem) { new (mem) SceneCommandLoadScene(); }
static void load_scene_cmd_dtor(void* mem) { ((SceneCommandLoadScene*)mem)->~SceneCommandLoadScene(); }
static void load_scene_cmd_apply(SceneCommand* cmd, Scene scene);
static void set_props_cmd_ctor(void* mem) { new (mem) SceneCommandSetProps(); }
static void set_props_cmd_dtor(void* mem) { ((SceneCommandSetProps*)mem)->~SceneCommandSetProps(); }
static void set_props_cmd_apply(SceneCommand* cmd, Scene scene);
static void get_props_cmd_ctor(void* mem) { new (mem) SceneCommandGetProps(); }
static void get_props_cmd_dtor(void* mem) { ((SceneCommandGetProps*)mem)->~SceneCommandGetProps(); }
static void get_props_cmd_apply(SceneCommand* cmd, Scene scene);
static void create_component_cmd_ctor(void* mem) { new (mem) SceneCommandCreateComponent(); }
static void create_component_cmd_dtor(void* mem) { ((SceneCommandCreateComponent*)mem)->~SceneCommandCreateComponent(); }
static void create_component_cmd_apply(SceneCommand* cmd, Scene scene);

static void load_scene_cmd_apply(SceneCommand* base, Scene scene)
{
    auto* cmd = (SceneCommandLoadScene*)base;

    cmd->loadSuccess = scene.load([&](SceneObj* sceneObj) -> bool {
        Scene scene(sceneObj);

        return (bool)scene.create_component_subtree(cmd->subtree);
    });
}

static void set_props_cmd_apply(SceneCommand* base, Scene scene)
{
    auto* cmd = (SceneCommandSetProps*)base;

    ComponentView compV = scene.get_component_by_path(cmd->compPath);
    if (!compV)
        return;

    const TypeMeta* compM = compV.type_meta();
    compM->apply_properties(compV.data(), compM->resolve(cmd->props));
}

static void get_props_cmd_apply(SceneCommand* base, Scene scene)
{
    auto* cmd = (SceneCommandGetProps*)base;

    ComponentView compV = scene.get_component_by_path(cmd->compPath);
    if (!compV)
    {
        cmd->props.clear();
        return;
    }

    const TypeMeta* compTypeMeta = compV.type_meta();

    if (cmd->props.empty()) // extract all properties
    {
        cmd->props = compTypeMeta->get_property_named_snapshot(compV.data());
        return;
    }

    // extract for each successfully resolved string path
    for (size_t i = 0; i < cmd->props.size(); i++)
    {
        uint32_t propIndex = 0;
        if (!compTypeMeta->resolve_property(cmd->props[i].name, propIndex) ||
            !compTypeMeta->getLocal(compV.data(), propIndex, 0, cmd->props[i].value))
            cmd->props[i].value = {};
    }
}

void create_component_cmd_apply(SceneCommand* base, Scene scene)
{
    auto* cmd = (SceneCommandCreateComponent*)base;
    cmd->result.compView = {};

    ComponentView parentV = scene.get_component_by_path(cmd->parentPath);
    if (!parentV)
        return;

    ComponentView compV = scene.create_component(cmd->compType, cmd->compName.c_str(), parentV.cuid());
    if (!compV)
        return;

    const TypeMeta* compM = compV.type_meta();

    Vector<PropertyValue> props(cmd->props.size());
    for (size_t i = 0; i < props.size(); i++)
    {
        const PropertyMeta* propM = compM->resolve_property(cmd->props[i].name, props[i].propIndex);
        if (!propM)
        {
            LD_DEBUG_BREAK;
            scene.destroy_component_subtree(compV.cuid());
            return;
        }

        props[i].arrayIndex = 0;
        props[i].value = cmd->props[i].value;
        if (!Value64::narrow(propM->valueType, props[i].value))
        {
            LD_DEBUG_BREAK;
            scene.destroy_component_subtree(compV.cuid());
            return;
        }
    }

    std::string err;
    if (compV.load_from_props(props, err))
        cmd->result.compView = compV;
}

static SceneCommandMeta sSceneCommandMeta[] = {
    {sizeof(SceneCommandLoadScene), &load_scene_cmd_ctor, &load_scene_cmd_dtor, &load_scene_cmd_apply, "load_scene"},
    {sizeof(SceneCommandSetProps), &set_props_cmd_ctor, &set_props_cmd_dtor, &set_props_cmd_apply, "set_props"},
    {sizeof(SceneCommandGetProps), &get_props_cmd_ctor, &get_props_cmd_dtor, &get_props_cmd_apply, "get_props"},
    {sizeof(SceneCommandCreateComponent), &create_component_cmd_ctor, &create_component_cmd_dtor, &create_component_cmd_apply, "create_component"},
};

static_assert(sizeof(sSceneCommandMeta) / sizeof(*sSceneCommandMeta) == SCENE_COMMAND_TYPE_ENUM_COUNT);

const char* SceneCommand::get_name(SceneCommandType type)
{
    return sSceneCommandMeta[(int)type].name;
}

SceneCommandType SceneCommand::get_type(const char* cstr)
{
    std::string name(cstr);

    for (int i = 0; i < (int)SCENE_COMMAND_TYPE_ENUM_COUNT; i++)
    {
        if (name == sSceneCommandMeta[i].name)
            return (SceneCommandType)i;
    }

    return SCENE_COMMAND_TYPE_ENUM_COUNT;
}

struct SceneCommandQueueObj
{
    LinearAllocator cmdLA = {};
    Queue<SceneCommand*> queue;
    SceneCommandFn responseCB = nullptr;
    void* user = nullptr;
};

SceneCommandQueue SceneCommandQueue::create()
{
    auto* obj = heap_new<SceneCommandQueueObj>(MEMORY_USAGE_SCENE);

    LinearAllocatorInfo laI{};
    laI.isMultiPage = true;
    laI.capacity = 1024; // larger than largest command size
    laI.usage = MEMORY_USAGE_SCENE;
    obj->cmdLA = LinearAllocator::create(laI);

    return SceneCommandQueue(obj);
}

void SceneCommandQueue::destroy(SceneCommandQueue queue)
{
    auto* obj = queue.unwrap();

    LinearAllocator::destroy(obj->cmdLA);

    heap_delete<SceneCommandQueueObj>(obj);
}

SceneCommand* SceneCommandQueue::enqueue(SceneCommandType type)
{
    const SceneCommandMeta& cmdM = sSceneCommandMeta[(int)type];

    SceneCommand* cmd = (SceneCommand*)mObj->cmdLA.allocate_aligned(cmdM.size, SCENE_COMMAND_ALIGNMENT);

    sSceneCommandMeta[(int)type].ctor(cmd);

    mObj->queue.push(cmd);

    return cmd;
}

void SceneCommandQueue::set_response_callback(SceneCommandFn responseCB, void* user)
{
    mObj->responseCB = responseCB;
    mObj->user = user;
}

void SceneCommandQueue::poll_commands(Scene& scene)
{
    while (!mObj->queue.empty())
    {
        SceneCommand* cmd = mObj->queue.front();
        mObj->queue.pop();

        sSceneCommandMeta[(int)cmd->type].apply(cmd, scene);

        if (mObj->responseCB)
            mObj->responseCB(cmd, mObj->user);
    }

    mObj->cmdLA.free();
}

} // namespace LD