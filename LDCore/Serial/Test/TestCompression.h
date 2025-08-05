#pragma once

#include "Extra/doctest/doctest.h"
#include <Ludens/Serial/Compress.h>
#include <string>
#include <vector>

using namespace LD;

namespace {

const char sTinyPayload[] = "tiny payload";

} // namespace

TEST_CASE("zstd")
{
    size_t size = strlen(sTinyPayload);
    size_t bound = LD::zstd_compress_bound(size);

    constexpr int compressionLevel = 3;
    std::vector<char> compressed(bound);
    size_t compressedSize = LD::zstd_compress(compressed.data(), compressed.size(), (const void*)sTinyPayload, size, compressionLevel);
    compressed.resize(compressedSize);

    std::string restore;
    restore.resize(size);

    LD::zstd_decompress(restore.data(), restore.size(), compressed.data(), compressed.size());

    CHECK(restore == std::string(sTinyPayload));
}

TEST_CASE("lz4")
{
    size_t size = strlen(sTinyPayload);
    size_t bound = LD::lz4_compress_bound(size);

    std::vector<char> compressed(bound);
    size_t compressedSize = LD::lz4_compress(compressed.data(), compressed.size(), (const void*)sTinyPayload, size);
    compressed.resize(compressedSize);

    std::string restore;
    restore.resize(size);

    LD::lz4_decompress(restore.data(), restore.size(), compressed.data(), compressed.size());

    CHECK(restore == std::string(sTinyPayload));
}