#pragma once

#include "Extra/doctest/doctest.h"
#include <Ludens/Serial/Compress.h>
#include <vector>

TEST_CASE("zstd")
{
    const char data[] = "tiny payload";
    size_t size = strlen(data);
    size_t bound = LD::zstd_compress_bound(size);

    std::vector<char> compressed(bound);
    size_t compressedSize = LD::zstd_compress(compressed.data(), compressed.size(), (const void*)data, size, 3);
    compressed.resize(compressedSize);

    std::string restore;
    restore.resize(size);

    LD::zstd_decompress(restore.data(), restore.size(), compressed.data(), compressed.size());

    CHECK(restore == "tiny payload");
}