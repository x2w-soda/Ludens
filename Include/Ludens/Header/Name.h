#pragma once

#include <Ludens/DSA/Hash.h>
#include <cstdint>
#include <cstring>
#include <string>

namespace LD {

/// @brief a string hash identifier
class Name
{
public:
    Name() : mHash(0) {}
    Name(std::string&& str)
        : mHash(hash32_FNV_1a(str.data(), str.size()))
    {
    }
    Name(const char* cstr)
        : mHash(hash32_FNV_1a(cstr, strlen(cstr)))
    {
    }

    inline operator uint32_t() const { return mHash; }
    inline bool operator==(const Name& other) const { return mHash == other.mHash; }
    inline bool operator!=(const Name& other) const { return mHash != other.mHash; }

private:
    uint32_t mHash;
};

} // namespace LD