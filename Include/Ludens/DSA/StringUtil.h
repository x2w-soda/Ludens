#pragma once

/// THIS IS A HEAVY HEADER, include this only from source files.
#include <Ludens/DSA/String.h>

#include <string>

namespace LD {

template <size_t TLocalSize, MemoryUsage TUsage>
std::ostream& operator<<(std::ostream& os, const TString<TLocalSize, TUsage>& str)
{
    return os.write((const char*)str.data(), (std::streamsize)str.size());
}

inline String to_string(const std::string& str)
{
    return String((const char8_t*)str.data(), str.size());
}

inline std::string to_std_string(const String& str)
{
    return std::string((const char*)str.data(), str.size());
}

inline std::string_view to_std_string_view(const String& str)
{
    return std::string_view((const char*)str.data(), str.size());
}

inline std::string_view to_std_string_view(View str)
{
    return std::string_view((const char*)str.data, str.size);
}

} // namespace LD
