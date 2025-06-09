#include <Ludens/Log/Log.h>
#include <Ludens/Media/Parser/WAV.h>
#include <Ludens/System/Memory.h>
#include <cstring>

namespace LD {

static_assert(sizeof(WAVHeader) == 44);

static Log sLog("MediaWAV");

struct WAVDataObj
{
    WAVHeader header;
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
        sLog.info("invalid input data");
        goto failure;
    }

    return {obj};

failure:
    heap_free(obj);
    return {};
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

uint32_t WAVData::get_sample_rate() const
{
    return mObj->header.sampleRate;
}


} // namespace LD