#include <Ludens/Header/Types.h>
#include <Ludens/Media/Format/WAV.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/System/FileSystem.h>

#include "../AudioDataObj.h"

namespace LD {

WAVData WAVData::create(const void* data, size_t size)
{
    AudioDataObj* obj = create_audio_data(data, size, AUDIO_DATA_FORMAT_WAV);

    return {obj};
}

void WAVData::destroy(WAVData data)
{
    AudioDataObj* obj = data.unwrap();

    destroy_audio_data(obj);
}

} // namespace LD