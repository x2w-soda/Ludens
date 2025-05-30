#include <Ludens/Serial/Compress.h>
#include <Ludens/Profiler/Profiler.h>
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

} // namespace LD