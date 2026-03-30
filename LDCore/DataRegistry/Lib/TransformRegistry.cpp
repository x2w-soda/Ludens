#include <Ludens/DSA/Queue.h>
#include <Ludens/DataRegistry/TransformRegistry.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Profiler/Profiler.h>

#include <utility>

#define TRANSFORM_REGISTRY_MEMORY_USAGE MEMORY_USAGE_MISC
#define TRANSFORM_REGISTRY_PAGE_SIZE 1024

namespace LD {

Transform2DRegistry::Transform2DRegistry()
{
    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(Transform2D);
    paI.isMultiPage = true;
    paI.usage = TRANSFORM_REGISTRY_MEMORY_USAGE;
    paI.pageSize = TRANSFORM_REGISTRY_PAGE_SIZE;
    mTransformPA = PoolAllocator::create(paI);
}

Transform2DRegistry::~Transform2DRegistry()
{
    for (auto* arr : mDepth)
        heap_delete<Depth>(arr);

    PoolAllocator::destroy(mTransformPA);
}

void Transform2DRegistry::invalidate_transforms()
{
    LD_PROFILE_SCOPE;

    if (mDepth.empty())
        return;

    Depth& rootDepth = *mDepth[0];
    size_t maxChildLI = rootDepth.local.size();

    for (size_t childLI = 0; childLI < maxChildLI; childLI++)
    {
        // SPACE: For a wide depth level we can split the range into jobs for the JobSystem,

        uint32_t childSI = rootDepth.local[childLI].id.index();

        mWorldMat4[childSI] = rootDepth.local[childLI].transform->as_mat4();
    }

    for (size_t d = 1; d < mDepth.size(); d++)
    {
        Depth& depthPrev = *mDepth[d - 1];
        Depth& depth = *mDepth[d];
        maxChildLI = depth.local.size();

        // SPACE: For a wide depth level we can split the range into jobs for the JobSystem,
        //        there is overhead for notifying std::condition_variable, so we probably
        //        don't want to split if a depth level takes only a few microseconds.

        for (size_t childLI = 0; childLI < maxChildLI; childLI++)
        {
            uint64_t parentSI = depth.local[childLI].parentID.index();

            Mat4 localMat4 = depth.local[childLI].transform->as_mat4();
            mWorldMat4[depth.local[childLI].id.index()] = mWorldMat4[parentSI] * localMat4;
        }
    }
}

bool Transform2DRegistry::has_transform(ID id)
{
    uint32_t transformSI = id.index();

    return id && transformSI < mSparse.size() && mSparse[transformSI].depthLevel >= 0;
}

void Transform2DRegistry::set_transform(ID id, const Transform2D& transform)
{
    const Sparse& sparse = mSparse[id.index()];

    Depth& depth = *mDepth[sparse.depthLevel];
    *depth.local[sparse.localIndex].transform = transform;
}

bool Transform2DRegistry::get_world_transform(ID id, Transform2D& outWorldTransform)
{
    if (!has_transform(id))
        return false;

    const Sparse& sparse = mSparse[id.index()];
    Depth& depth = *mDepth[sparse.depthLevel];
    const Entry& entry = depth.local[sparse.localIndex];

    Transform2D parentWorld;
    if (!get_world_transform(entry.parentID, parentWorld))
        parentWorld = Transform2D::identity();

    outWorldTransform = Transform2D::child_world(*entry.transform, parentWorld);
    return true;
}

Transform2D* Transform2DRegistry::create(ID childID, ID parentID)
{
    uint32_t childSI = childID.index();
    uint32_t parentSI = parentID.index();
    uint32_t maxSI = std::max(childSI, parentSI);
    int childDepthLevel = 0;

    if (maxSI >= mSparse.size())
    {
        mSparse.resize(maxSI + 1);
        mWorldMat4.resize(maxSI + 1);
    }

    if (parentID)
    {
        Sparse& sparse = mSparse[parentSI];
        childDepthLevel = sparse.depthLevel + 1;
    }

    reserve_depth(childDepthLevel + 1);

    Transform2D* childTransform;
    Depth& childDepth = *mDepth[childDepthLevel];
    uint64_t childLI = childDepth.local.size();
    childDepth.local.resize(childLI + 1);
    childDepth.local[childLI].transform = childTransform = (Transform2D*)mTransformPA.allocate();
    childDepth.local[childLI].parentID = parentID;
    childDepth.local[childLI].id = childID;

    Sparse& childS = mSparse[childSI];
    childS.depthLevel = childDepthLevel;
    childS.localIndex = childLI;

    // stable address until destroyed
    *childTransform = {Vec2(0.0f), Vec2(1.0f), 0.0f};
    return childTransform;
}

void Transform2DRegistry::destroy_subtree(ID rootID, IDHierarchyCallback hierarchyCB, void* user)
{
    // only validate root, after this we assume hierarchy callback is correct.
    uint64_t rootSI = rootID.index();
    if (!rootID || !hierarchyCB || rootSI >= mSparse.size() || mSparse[rootSI].depthLevel < 0)
        return;

    Vector<ID> childrenID;
    Queue<ID> work;
    work.push(rootID);

    while (!work.empty())
    {
        ID id = work.front();
        work.pop();

        childrenID.clear();
        hierarchyCB(id, childrenID, user);

        for (ID childID : childrenID)
        {
            LD_ASSERT(childID);
            work.push(childID);
        }

        Transform2D* popTransform = swap_and_pop(id);
        mTransformPA.free(popTransform);
    }
}

void Transform2DRegistry::reparent_subtree(ID childID, ID parentID, IDHierarchyCallback hierarchyCB, void* user)
{
    Vector<ID> childrenID;
    Queue<std::pair<ID, ID>> work;
    work.emplace(childID, parentID);

    while (!work.empty())
    {
        std::pair<ID, ID> pair = work.front();
        work.pop();

        childID = pair.first;
        parentID = pair.second;

        childrenID.clear();
        hierarchyCB(childID, childrenID, user);
        for (ID id : childrenID)
            work.emplace(id, childID);

        int parentDepthLevel = -1;
        int childDepthLevel = 0;
        Depth* parentDepth = nullptr;
        Depth* childNewDepth = nullptr;
        uint32_t childSI = childID.index();
        Sparse& childS = mSparse[childSI];

        if (parentID)
        {
            parentDepthLevel = mSparse[parentID.index()].depthLevel;
            parentDepth = mDepth[parentDepthLevel];
            childDepthLevel = parentDepthLevel + 1;
            reserve_depth(childDepthLevel + 1);
            childNewDepth = mDepth[childDepthLevel];
        }
        else
        {
            // child becomes a root
            childNewDepth = mDepth[0];
        }

        Transform2D* childTransform = swap_and_pop(childID);

        size_t childNewLI = childNewDepth->local.size();
        childNewDepth->local.resize(childNewLI + 1);
        childNewDepth->local[childNewLI].id = childID;
        childNewDepth->local[childNewLI].parentID = parentID;
        childNewDepth->local[childNewLI].transform = childTransform;

        childS.depthLevel = childDepthLevel;
        childS.localIndex = childNewLI;
    }
}

void Transform2DRegistry::reserve_depth(int depth)
{
    if (depth < mDepth.size())
        return;

    size_t size = (size_t)mDepth.size();
    mDepth.resize(depth);

    for (size_t i = size; i < depth; i++)
    {
        mDepth[i] = heap_new<Depth>(TRANSFORM_REGISTRY_MEMORY_USAGE);
    }
}

Transform2D* Transform2DRegistry::swap_and_pop(ID popID)
{
    uint32_t popSI = popID.index();
    Sparse& popS = mSparse[popSI];
    Depth& depth = *mDepth[popS.depthLevel];
    uint32_t popLI = popS.localIndex;

    Transform2D* popTransform = depth.local[popLI].transform;

    uint32_t lastLI = depth.local.size() - 1;
    uint32_t lastSI = depth.local[lastLI].id.index();

    Entry entry = depth.local[popLI];

    depth.local[popLI] = depth.local[lastLI];
    depth.local.resize(lastLI);

    mSparse[lastSI].localIndex = popLI;
    mSparse[popSI].depthLevel = -1;
    mSparse[popSI].localIndex = 0;

    return popTransform;
}

} // namespace LD