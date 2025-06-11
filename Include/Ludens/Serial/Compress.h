#pragma once

#include <cstddef>

namespace LD {

size_t zstd_compress_bound(size_t srcSize);

size_t zstd_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize, int compressionLevel);

void zstd_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize);

size_t lz4_compress_bound(size_t srcSize);

size_t lz4_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize);

void lz4_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize);

} // namespace LD
