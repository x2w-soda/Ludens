#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/View.h>
#include <Ludens/Serial/Value.h>

namespace LD {

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

/// @brief A snapshot of a single property value.
struct PropertyValue
{
    Value64 value;
    uint32_t index; // property index within domain, leads to value type
};

/// @brief A snapshot of a delta property changes.
struct PropertyDelta
{
    Value64 oldValue;
    Value64 newValue;
    uint32_t index; // property index within domain, leads to value type
};

/// @brief A single Property entry in some object.
struct PropertyMeta
{
    const char* name;   // unique name within type
    ValueType type;     // property value type
    PropertyFlagBit flags;
    PropertyUIHint uiHint;
    Value64 defaultVal; // default property value hint
};

struct PropertyMetaTable
{
    void* user;
    const PropertyMeta* entries;
    size_t entryCount;
    void (*getter)(void* obj, uint32_t index, Value64& val);
    void (*setter)(void* obj, uint32_t index, const Value64& val);
    
    Vector<PropertyValue> get_property_snapshot(void* obj) const;
    Vector<PropertyDelta> get_property_delta(void* obj, const Vector<PropertyValue>& oldProps, const Vector<PropertyValue>& newProps) const;
    void apply_properties(void* obj, const Vector<PropertyValue>& props) const;
    void apply_old_properties(void* obj, const Vector<PropertyDelta>& delta) const;
    void apply_new_properties(void* obj, const Vector<PropertyDelta>& delta) const;
};

} // namespace LD