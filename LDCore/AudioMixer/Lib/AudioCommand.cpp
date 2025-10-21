#include <Ludens/AudioMixer/AudioCommand.h>
#include <Ludens/System/Memory.h>
#include <atomic>

// Instead of trying to come up with a (hopefully correct) lock free queue ourselves,
// maybe it's better to just use existing battle tested solutions.
// https://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++
#include <readerwriterqueue.h>

namespace LD {

/// @brief Use a lock-free queue that the audio thread can dequeue (or fail to dequeue)
///        in a bounded amount of time.
struct AudioCommandQueueObj
{
    AudioCommandQueueObj(size_t capacity)
        : lockFreeQueue(capacity) {}

    moodycamel::ReaderWriterQueue<AudioCommand> lockFreeQueue;
};

AudioCommandQueue AudioCommandQueue::create(const AudioCommandQueueInfo& info)
{
    auto* obj = heap_new<AudioCommandQueueObj>(MEMORY_USAGE_AUDIO, info.capacity);

    return AudioCommandQueue(obj);
}

void AudioCommandQueue::destroy(AudioCommandQueue queue)
{
    auto* obj = (AudioCommandQueueObj*)queue.unwrap();

    heap_delete<AudioCommandQueueObj>(obj);
}

bool AudioCommandQueue::enqueue(const AudioCommand& cmd)
{
    return mObj->lockFreeQueue.try_enqueue(cmd);
}

bool AudioCommandQueue::dequeue(AudioCommand& cmd)
{
    return mObj->lockFreeQueue.try_dequeue(cmd);
}

} // namespace LD