#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/RenderBackend/RUID.h>
#include <Ludens/Serial/SUID.h>

#define COMPONENT_PROP_TRANSFORM 0

namespace LD {

struct ComponentBase;
struct TransformEx;
struct Transform2D;
struct PropertyValue;
struct TypeMeta;

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
    const TypeMeta* type_meta();
    ComponentType type();
    CUID cuid();
    SUID suid();
    RUID ruid();

    bool load_from_props(const Vector<PropertyValue>& props, std::string& err);

    const char* get_name();
    void set_name(const char* cstr);
    AssetID get_asset_id(uint32_t assetSlotIndex);
    bool set_asset_id(uint32_t assetSlotIndex, AssetID id);
    AssetType get_asset_type(uint32_t assetSlotIndex);
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

struct ComponentData
{
    std::string name;
    SUID suid = {};
    ComponentType type;
    Vector<PropertyValue> props;
    int32_t parentIndex = -1;
};

struct ComponentSubtreeData
{
    Vector<ComponentData> components;
};

} // namespace LD