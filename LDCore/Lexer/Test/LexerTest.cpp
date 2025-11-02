#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/Lexer/Unicode.h>

#define ARRAY_LEN(A) (sizeof(A) / sizeof(*A))

using namespace LD;

int check_codepoints(const uint8_t* utf8, const uint32_t* codes, size_t codeCount)
{
    uint32_t state = UTF8_ACCEPT;
    uint32_t code;

    int count = 0;
    for (const uint8_t* s = utf8; *s; s++)
    {
        if (utf8_decode(&state, &code, *s) == UTF8_ACCEPT && count < codeCount)
        {
            CHECK(code == codes[count++]);
        }
    }

    return count;
}

TEST_CASE("UTF8 codepoints")
{
    const char8_t s1[] = u8"hello world";
    const uint32_t s1codes[] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
    int count = check_codepoints((const uint8_t*)s1, s1codes, ARRAY_LEN(s1codes));
    CHECK(count == ARRAY_LEN(s1codes));

    const char8_t s2[] = u8"😀 🎉";
    const uint32_t s2codes[] = {0x1F600, ' ', 0x1F389};
    count = check_codepoints((const uint8_t*)s2, s2codes, ARRAY_LEN(s2codes));
    CHECK(count == ARRAY_LEN(s2codes));

    const char8_t s3[] = u8"Hi 你好";
    const uint32_t s3codes[] = {'H', 'i', ' ', 0x4F60, 0x597D};
    count = check_codepoints((const uint8_t*)s3, s3codes, ARRAY_LEN(s3codes));
    CHECK(count == ARRAY_LEN(s3codes));
}

TEST_CASE("UTF8 decode line")
{
    const char8_t s1[] = u8"";
    uint32_t lineLen = utf8_decode_line((const uint8_t*)s1, sizeof(s1) - 1);
    CHECK(lineLen == 0);

    const char8_t s2[] = u8"\n";
    lineLen = utf8_decode_line((const uint8_t*)s2, sizeof(s2) - 1);
    CHECK(lineLen == 0);

    // windows CRLF
    const char8_t s3[] = u8"\r\n";
    lineLen = utf8_decode_line((const uint8_t*)s3, sizeof(s3) - 1);
    CHECK(lineLen == 0);

    const char8_t s4[] = u8"non-empty\r\nNext-line";
    lineLen = utf8_decode_line((const uint8_t*)s4, sizeof(s4) - 1);
    CHECK(lineLen == 9);

    const char8_t s5[] = u8"\t\tHi 你好\nNext-line";
    lineLen = utf8_decode_line((const uint8_t*)s5, sizeof(s5) - 1);
    CHECK(lineLen == 11);
}

TEST_CASE("UTF8 decode whitespace")
{
    const char8_t s1[] = u8"";
    uint32_t lineLen = utf8_decode_whitespace((const uint8_t*)s1, sizeof(s1) - 1);
    CHECK(lineLen == 0);

    const char8_t s2[] = u8"\n";
    lineLen = utf8_decode_whitespace((const uint8_t*)s2, sizeof(s2) - 1);
    CHECK(lineLen == 1);

    const char8_t s3[] = u8"\t \n\rtext";
    lineLen = utf8_decode_whitespace((const uint8_t*)s3, sizeof(s3) - 1);
    CHECK(lineLen == 4);

    const char8_t s4[] = u8"!\t";
    lineLen = utf8_decode_whitespace((const uint8_t*)s4, sizeof(s4) - 1);
    CHECK(lineLen == 0);
}