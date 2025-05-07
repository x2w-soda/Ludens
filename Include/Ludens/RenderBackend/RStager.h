#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <vector>

namespace LD {

/// @brief Utility class to upload CPU data to GPU-local memory via staging buffers
class RStager
{
public:
    RStager() = delete;
    RStager(const RStager&) = delete;
    RStager(RDevice device, RQueueType queueType);
    ~RStager();

    RStager& operator=(const RStager&) = delete;

    /// @brief Add a command to upload buffer memory.
    void add_buffer_data(RBuffer dst, const void* data);

    /// @brief Add a command to upload image memory. Performing a layout transition after the transfer.
    void add_image_data(RImage dst, const void* data, RImageLayout finalLayout);

    /// @brief Begin uploading, blocks until transfer queue has completed submission.
    void submit(RQueue transferQueue);

private:
    RDevice mDevice;
    RCommandPool mPool;
    RCommandList mList;
    std::vector<RBuffer> mStagingBuffers;
};

} // namespace LD