#include <Ludens/Serial/Property.h>

namespace LD {

Vector<PropertyValue> TypeMeta::get_property_snapshot(void* obj) const
{
    Vector<PropertyValue> snapshot(propCount);

    // Array and Vector elements not captured
    const uint32_t arrayIndex = 0;

    for (size_t i = 0; i < propCount; i++)
    {
        snapshot[i].propIndex = (uint32_t)i;
        (void)getLocal(obj, snapshot[i].propIndex, arrayIndex, snapshot[i].value);
    }

    return snapshot;
}

Vector<PropertyDelta> TypeMeta::get_property_delta(void* obj, const Vector<PropertyValue>& oldProps, const Vector<PropertyValue>& newProps) const
{
    Vector<PropertyDelta> delta;
    size_t i = 0;
    size_t j = 0;

    delta.reserve(propCount);

    while (i != oldProps.size() && j != newProps.size())
    {
        size_t oldI = oldProps[i].propIndex;
        size_t newI = newProps[j].propIndex;

        if (oldI < newI)
        {
            i++;
            continue;
        }
        else if (oldI > newI)
        {
            j++;
            continue;
        }

        if (oldProps[i].value != newProps[j].value)
            delta.emplace_back(oldI, 0, oldProps[i].value, newProps[j].value);

        i++;
        j++;
    }

    return delta;
}

void TypeMeta::apply_properties(void* obj, const Vector<PropertyValue>& newProps) const
{
    for (const PropertyValue& prop : newProps)
        (void)setLocal(obj, prop.propIndex, 0, prop.value);
}

void TypeMeta::apply_old_properties(void* obj, const Vector<PropertyDelta>& newProps) const
{
    for (const PropertyDelta& prop : newProps)
        (void)setLocal(obj, prop.propIndex, 0, prop.oldValue);
}

void TypeMeta::apply_new_properties(void* obj, const Vector<PropertyDelta>& newProps) const
{
    for (const PropertyDelta& prop : newProps)
        (void)setLocal(obj, prop.propIndex, 0, prop.newValue);
}

} // namespace LD