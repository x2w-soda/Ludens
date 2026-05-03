#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/View.h>
#include <Ludens/Serial/Value.h>

namespace LD {

struct TypeMeta;

using PropertyFlagBits = uint32_t;
enum PropertyFlagBit : PropertyFlagBits
{
    PROPERTY_FLAG_NORMALIZED_BIT = 1,
};

enum PropertyUIHint
{
    PROPERTY_UI_HINT_DEFAULT = 0,
    PROPERTY_UI_HINT_ASSET,
    PROPERTY_UI_HINT_DROPDOWN,
    PROPERTY_UI_HINT_SLIDER,
};

struct PropertyValue
{
    uint32_t propIndex;
    uint32_t arrayIndex;
    Value64 value;
};

struct PropertyPathValue
{
    Vector<uint32_t> path;
    Value64 value;
};

struct PropertyDelta
{
    uint32_t propIndex;
    uint32_t arrayIndex;
    Value64 oldValue;
    Value64 newValue;
};

struct PropertyPathDelta
{
    enum VectorEdit : uint32_t
    {
        Insert, // path stops at vector index
        Remove, // path stops at vector index
        Assign, // path stops at vector element
    } vectorEdit;

    Vector<uint32_t> path;
    Value64 oldValue;
    Value64 newValue;
};

/// @brief A single Property entry in some Type.
struct PropertyMeta
{
    const char* name;      // property name within Type
    TypeMeta* type;        // if not null, the Type of the property
    ValueType valueType;   // property value type
    Value64 valueDefault;  // property default value hint
    PropertyUIHint uiHint; // UI display hint
    PropertyFlagBit flags; // property flags
};

/// @brief Type reflection of some object.
struct TypeMeta
{
    const char* name;          // type name
    const PropertyMeta* props; // property entries
    size_t propCount;          // property entry count
    bool (*getLocal)(void* obj, uint32_t propIndex, uint32_t arrayIndex, Value64& val);
    bool (*setLocal)(void* obj, uint32_t propIndex, uint32_t arrayIndex, const Value64& val);
    int32_t (*getSize)(void* obj, uint32_t propIndex);

    Vector<PropertyValue> get_property_snapshot(void* obj) const;
    Vector<PropertyDelta> get_property_delta(void* obj, const Vector<PropertyValue>& oldProps, const Vector<PropertyValue>& newProps) const;
    void apply_properties(void* obj, const Vector<PropertyValue>& props) const;
    void apply_old_properties(void* obj, const Vector<PropertyDelta>& delta) const;
    void apply_new_properties(void* obj, const Vector<PropertyDelta>& delta) const;
};

} // namespace LD