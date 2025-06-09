#include <Ludens/Log/Log.h>
#include <Ludens/Media/Parser/WAV.h>
#include <Ludens/System/Memory.h>
#include <cstring>

namespace LD {

static_assert(sizeof(WAVHeader) == 36);

static Log sLog("MediaWAV");

struct WAVDataObj
{
    WAVHeader header;
    uint64_t dataOffset; /// byte offset to sample data
    uint64_t dataSize;   /// sample data size in bytes
};

WAVData WAVData::create(const void* data, size_t size)
{
    if (size < sizeof(WAVHeader))
    {
        sLog.info("input size too small");
        return {};
    }

    WAVDataObj* obj = (WAVDataObj*)heap_malloc(size, MEMORY_USAGE_MEDIA);
    memcpy(obj, data, size);

    const WAVHeader& header = obj->header;
    if (strncmp((const char*)header.fileTypeBlocID, "RIFF", 4) || strncmp((const char*)header.fileFormatID, "WAVE", 4))
    {
        sLog.error("invalid input data");
        heap_free(obj);
        return {};
    }

    const char* blockID = (const char*)(&header + 1);
    uint32_t blockSize = *(uint32_t*)(blockID + 4);
    uint32_t blockOffset = sizeof(WAVHeader);

    while (blockOffset < size && strncmp(blockID, "data", 4))
    {
        blockOffset += blockSize + 8;
        blockID = (const char*)&header + blockOffset;
        blockSize = *(uint32_t*)(blockID + 4);
    }

    if (blockOffset >= size)
    {
        sLog.error("data chunk not found");
        heap_free(obj);
        return {};
    }

    obj->dataOffset = blockOffset + 8;
    obj->dataSize = (uint64_t)blockSize;

    return {obj};
}

void WAVData::destroy(WAVData data)
{
    WAVDataObj* obj = data;

    heap_free(obj);
}

void WAVData::get_header(WAVHeader& header) const
{
    memcpy(&header, &mObj->header, sizeof(WAVHeader));
}

const void* WAVData::get_data(uint64_t& byteSize) const
{
    byteSize = (uint64_t)mObj->dataSize;
    return (const char*)mObj + mObj->dataOffset;
}

uint32_t WAVData::get_channels() const
{
    return (uint32_t)mObj->header.channelCount;
}

uint32_t WAVData::get_sample_rate() const
{
    return (uint32_t)mObj->header.sampleRate;
}

uint32_t WAVData::get_bits_per_sample() const
{
    return (uint32_t)mObj->header.bitsPerSample;
}

} // namespace LD