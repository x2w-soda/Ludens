#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataRegistry.h>

namespace LD {

struct ComponentBase;
struct TransformEx;
struct Transform2D;

/// @brief Public interface for all components.
class ComponentView
{
public:
    ComponentView() = default;
    ComponentView(ComponentBase** data)
        : mData(data) {}

    /// @brief Check for interface validity before calling any methods.
    inline operator bool() const noexcept { return mData != nullptr; }

    inline ComponentBase* base() { return *mData; }
    inline ComponentBase** data() { return mData; }
    ComponentType type();
    CUID cuid();
    SUID suid();
    RUID ruid();

    const char* get_name();
    AssetID get_script_asset_id();
    void set_script_asset_id(AssetID assetID);
    void get_children(Vector<ComponentView>& children);
    ComponentView get_parent();

    bool get_transform(TransformEx& transform);
    bool set_transform(const TransformEx& transform);
    bool get_transform_2d(Transform2D& transform);
    bool set_transform_2d(const Transform2D& transform);

    /// @note Slow path for editor. 
    bool get_world_transform_2d(Transform2D& transform);

    bool get_world_mat4(Mat4& worldMat4);

protected:
    ComponentBase** mData = nullptr;
};

} // namespace LD