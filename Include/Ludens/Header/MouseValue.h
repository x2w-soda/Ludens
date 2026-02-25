#pragma once

#include <Ludens/Header/KeyCode.h>

#include <utility>

namespace LD {

struct MouseValue
{
    uint32_t u32;

    MouseValue() : u32(0) {}
    MouseValue(MouseButton button)
        : u32(button) {}
    MouseValue(MouseButton button, KeyMods mods)
        : u32((mods << 16) | button) {}

    KeyMods mods() { return static_cast<KeyMods>(u32 >> 16); }
    MouseButton button() { return static_cast<MouseButton>(u32 & 0xFFFF); }

    bool operator<(const MouseValue& other) const { return u32 < other.u32; }
    bool operator==(const MouseValue& other) const { return u32 == other.u32; }
};

} // namespace LD

namespace std {
template <>
struct hash<LD::MouseValue>
{
    size_t operator()(const LD::MouseValue& value) const
    {
        return std::hash<uint32_t>{}(value.u32);
    }
};
} // namespace std