#include <Ludens/AudioBackend/AudioBackend.h>

namespace LD {

void AudioObject::set_acquired(bool acquired)
{
    mAudioThreadAcquired.store(acquired);
}

bool AudioObject::is_acquired()
{
    return mAudioThreadAcquired.load();
}

AudioHandle::AudioHandle(AudioObject* obj)
{
    mObj = obj;
}

void AudioHandle::acquire()
{
    mObj->set_acquired(true);
}

void AudioHandle::release()
{
    mObj->set_acquired(false);
}

bool AudioHandle::is_acquired()
{
    return mObj->is_acquired();
}

} // namespace LD