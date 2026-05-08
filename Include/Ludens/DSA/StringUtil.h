#pragma once

#include <Ludens/DSA/String.h>

#include <format>
#include <ostream>

namespace LD {

template <size_t TLocalSize, MemoryUsage TUsage>
std::ostream& operator<<(std::ostream& os, const TString<TLocalSize, TUsage>& str)
{
    return os.write((const char*)str.data(), (std::streamsize)str.size());
}

} // namespace LD

template <size_t TLocalSize, LD::MemoryUsage TUsage>
struct std::formatter<LD::TString<TLocalSize, TUsage>> : std::formatter<std::string_view>
{
    auto format(const LD::TString<TLocalSize, TUsage>& str, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(std::string_view((const char*)str.data(), str.size()), ctx);
    }
};
