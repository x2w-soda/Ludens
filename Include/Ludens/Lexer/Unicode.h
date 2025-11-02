#pragma once

#include <cctype>
#include <cstdint>

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

namespace LD {

extern const uint8_t eUTF8d[];

// https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
inline uint32_t utf8_decode(uint32_t* state, uint32_t* code, uint32_t byte)
{
    uint32_t type = eUTF8d[byte];

    *code = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*code << 6) : (0xff >> type) & (byte);

    *state = eUTF8d[256 + *state * 16 + type];
    return *state;
}

inline uint32_t utf8_decode_line(const uint8_t* utf8, size_t len)
{
    uint32_t state = UTF8_ACCEPT;
    uint32_t code;

    for (size_t i = 0; i < len; i++)
    {
        utf8_decode(&state, &code, utf8[i]);

        switch (state)
        {
        case UTF8_REJECT:
            return 0;
        case UTF8_ACCEPT:
            if (code == '\n' || (code == '\r' && i + 1 < len && utf8[i + 1] == '\n'))
                return i;
        }
    }

    return len;
}

inline uint32_t utf8_decode_whitespace(const uint8_t* utf8, size_t len)
{
    uint32_t state = UTF8_ACCEPT;
    uint32_t code;

    for (size_t i = 0; i < len; i++)
    {
        utf8_decode(&state, &code, utf8[i]);

        switch (state)
        {
        case UTF8_REJECT:
            return 0;
        case UTF8_ACCEPT:
            if (code >= 128 || !isspace((int)code))
                return i;
        }
    }

    return len;
}

} // namespace LD