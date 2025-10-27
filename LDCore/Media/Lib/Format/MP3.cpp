#include <Ludens/Header/Types.h>
#include <Ludens/Media/Format/MP3.h>
#include <Ludens/System/FileSystem.h>
#include "../AudioDataObj.h"

namespace LD {

MP3Data MP3Data::create(const void* data, size_t size)
{
    AudioDataObj* obj = create_audio_data(data, size, AUDIO_DATA_FORMAT_MP3);

    return {obj};
}

void MP3Data::destroy(MP3Data data)
{
    AudioDataObj* obj = data.unwrap();

    destroy_audio_data(obj);
}

} // namespace LD