#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Compress.h>
#include <iostream>
#include <lz4.h>  // hide
#include <zstd.h> // hide

namespace LD {

size_t zstd_compress_bound(size_t srcSize)
{
    return ::ZSTD_compressBound(srcSize);
}

size_t zstd_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize, int compressionLevel)
{
    LD_PROFILE_SCOPE;

    return ::ZSTD_compress(dst, dstCapacity, src, srcSize, compressionLevel);
}

void zstd_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize)
{
    LD_PROFILE_SCOPE;

    ::ZSTD_decompress(dst, dstCapacity, src, compressedSize);
}

size_t lz4_compress_bound(size_t srcSize)
{
    return (size_t)::LZ4_compressBound((int)srcSize);
}

size_t lz4_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize)
{
    LD_PROFILE_SCOPE;

    return (size_t)::LZ4_compress_default((const char*)src, (char*)dst, (int)srcSize, (int)dstCapacity);
}

void lz4_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize)
{
    LD_PROFILE_SCOPE;

    int result = ::LZ4_decompress_safe((const char*)src, (char*)dst, (int)compressedSize, (int)dstCapacity);
    // TODO: validate
}

} // namespace LD