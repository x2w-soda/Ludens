#pragma once

#include <Ludens/Header/Handle.h>
#include <atomic>

namespace LD {

/// @brief Audio object base class. Heap allocations of these objects
///        are done on the main thread. Ownership is transfered to
///        audio thread after acquire and before release.
class AudioObject
{
public:
    void set_acquired(bool acquired);
    bool is_acquired();

private:
    std::atomic_bool mAudioThreadAcquired;
};

struct AudioHandle : public Handle<AudioObject>
{
    AudioHandle() = default;
    AudioHandle(AudioObject* obj);

    /// @brief Called by audio thread to acquire underyling audio resource.
    ///        Main thread should no longer access this resource.
    void acquire();

    /// @brief Called by audio thread to release underyling audio resource.
    ///        Main thread may proceed with accessing or destroying this resource.
    void release();

    /// @brief Check atomically if the resource is currently owned by audio thread.
    bool is_acquired();
};

} // namespace LD