#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/IDRegistry.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Memory/Allocator.h>

namespace LD {

struct Transform2D;

typedef void (*IDHierarchyCallback)(ID id, Vector<ID>& childrenID, void* user);

class Transform2DRegistry
{
public:
    Transform2DRegistry();
    ~Transform2DRegistry();
    Transform2DRegistry(const Transform2DRegistry&) = delete;

    Transform2DRegistry& operator=(const Transform2DRegistry&) = delete;

    void invalidate_transforms();

    bool has_transform(ID id);
    void set_transform(ID id, const Transform2D& transform);

    inline Mat4 get_world_mat4(ID id)
    {
        return mWorldMat4[id.index()];
    }

    Transform2D* create(ID id, ID parentID);
    void destroy(ID id, IDHierarchyCallback hierarchyCB, void* user);
    void reparent(ID id, ID parentID, IDHierarchyCallback hierarchyCB, void* user);

private:
    struct Entry
    {
        ID id;
        ID parentID;
        Transform2D* transform;
    };

    struct Sparse
    {
        int depthLevel = -1;
        uint32_t localIndex; // index within depth level
    };

    struct Depth
    {
        Vector<Entry> local;
    };

    void reserve_depth(int depth);
    Entry swap_and_pop(ID popID);

private:
    Vector<Depth*> mDepth;
    Vector<Sparse> mSparse;
    Vector<Mat4> mWorldMat4;
    PoolAllocator mTransformPA{};
};

} // namespace LD