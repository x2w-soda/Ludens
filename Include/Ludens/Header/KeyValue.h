#pragma once

#include <Ludens/Header/KeyCode.h>

#include <cstdint>
#include <functional>

namespace LD {

struct KeyValue
{
    uint32_t u32;

    KeyValue() : u32(0) {}
    KeyValue(KeyCode code)
        : u32(code) {}
    KeyValue(KeyCode code, KeyMods mods)
        : u32((mods << 16) | code) {}

    KeyMods mods() { return static_cast<KeyMods>(u32 >> 16); }
    KeyCode code() { return static_cast<KeyCode>(u32 & 0xFFFF); }

    bool operator<(const KeyValue& other) const { return u32 < other.u32; }
    bool operator==(const KeyValue& other) const { return u32 == other.u32; }
};

} // namespace LD

namespace std {

template <>
struct hash<LD::KeyValue>
{
    size_t operator()(const LD::KeyValue& value) const
    {
        return hash<uint32_t>{}(value.u32);
    }
};

} // namespace std
