#include <Ludens/Serial/Property.h>

namespace LD {

Vector<PropertyValue> PropertyMetaTable::get_property_snapshot(void* obj) const
{
    Vector<PropertyValue> snapshot(entryCount);

    for (size_t i = 0; i < entryCount; i++)
    {
        snapshot[i].index = i;
        getter(obj, i, snapshot[i].value);
    }

    return snapshot;
}

Vector<PropertyDelta> PropertyMetaTable::get_property_delta(void* obj, const Vector<PropertyValue>& oldProps, const Vector<PropertyValue>& newProps) const
{
    Vector<PropertyDelta> delta;
    size_t i = 0;
    size_t j = 0;

    delta.reserve(entryCount);

    while (i != oldProps.size() && j != newProps.size())
    {
        size_t oldI = oldProps[i].index;
        size_t newI = newProps[j].index;

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
            delta.emplace_back(oldProps[i].value, newProps[j].value, oldI);

        i++;
        j++;
    }

    return delta;
}

void PropertyMetaTable::apply_properties(void* obj, const Vector<PropertyValue>& props) const
{
    for (const PropertyValue& prop : props)
        setter(obj, prop.index, prop.value);
}

void PropertyMetaTable::apply_old_properties(void* obj, const Vector<PropertyDelta>& props) const
{
    for (const PropertyDelta& prop : props)
        setter(obj, prop.index, prop.oldValue);
}

void PropertyMetaTable::apply_new_properties(void* obj, const Vector<PropertyDelta>& props) const
{
    for (const PropertyDelta& prop : props)
        setter(obj, prop.index, prop.newValue);
}

} // namespace LD