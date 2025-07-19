#include <Ludens/Header/Types.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Media/Format/WAV.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Memory.h>
#include <cstring>
#include <vector>

namespace fs = std::filesystem;

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

SampleFormat WAVData::get_sample_format() const
{
    if (mObj->header.audioFormat == 1) // PCM
    {
        switch (mObj->header.bitsPerSample)
        {
        case 8:
            return SAMPLE_FORMAT_U8;
        case 16:
            return SAMPLE_FORMAT_S16;
        case 24:
            return SAMPLE_FORMAT_S24;
        case 32:
            return SAMPLE_FORMAT_S32;
        default:
            return SAMPLE_FORMAT_UNKNOWN;
        }
    }

    if (mObj->header.audioFormat == 3) // IEEE-754
    {
        switch (mObj->header.bitsPerSample)
        {
        case 32:
            return SAMPLE_FORMAT_F32;
        default:
            return SAMPLE_FORMAT_UNKNOWN;
        }
    }

    return SAMPLE_FORMAT_UNKNOWN;
}

uint32_t WAVData::get_sample_count() const
{
    return mObj->dataSize / (mObj->header.bitsPerSample / 8);
}

uint32_t WAVData::get_sample_rate() const
{
    return (uint32_t)mObj->header.sampleRate;
}

uint32_t WAVData::get_bits_per_sample() const
{
    return (uint32_t)mObj->header.bitsPerSample;
}

bool WAVData::save_to_disk(const fs::path& path, const WAVHeader& header, const void* data, uint64_t dataSize)
{
    size_t wavDataSize = sizeof(WAVHeader) + 8 + dataSize;
    std::vector<byte> wavData(wavDataSize);

    memcpy(wavData.data(), &header, sizeof(WAVHeader));
    char* blockID = (char*)(wavData.data() + sizeof(WAVHeader));
    uint32_t* blockSize = (uint32_t*)(blockID + 4);

    // store samples at data chunk
    strncpy(blockID, "data", 4);
    *blockSize = dataSize;
    memcpy(blockID + 8, data, dataSize);

    return FS::write_file(path, wavData.size(), wavData.data());
}

} // namespace LD