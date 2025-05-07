#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>

namespace LD {

RStager::RStager(RDevice device, RQueueType queueType)
    : mDevice(device)
{
    mPool = device.create_command_pool({.queueType = queueType, .hintTransient = true});
    mList = mPool.allocate();
    mList.begin();
}

RStager::~RStager()
{
    mDevice.destroy_command_pool(mPool);

    for (RBuffer stagingBuffer : mStagingBuffers)
        mDevice.destroy_buffer(stagingBuffer);
}

void RStager::add_buffer_data(RBuffer dst, const void* data)
{
    uint64_t size = dst.size();

    RBuffer stagingBuffer = mDevice.create_buffer({.usage = RBUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   .size = size,
                                                   .hostVisible = true});

    stagingBuffer.map();
    stagingBuffer.map_write(0, size, data);
    stagingBuffer.unmap();

    mStagingBuffers.push_back(stagingBuffer);

    RBufferCopy region{.size = size};
    mList.cmd_copy_buffer(stagingBuffer, dst, 1, &region);
}

void RStager::add_image_data(RImage dst, const void* data, RImageLayout finalLayout)
{
    uint64_t imageSize = dst.size();

    RBuffer stagingBuffer = mDevice.create_buffer({.usage = RBUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   .size = imageSize,
                                                   .hostVisible = true});

    stagingBuffer.map();
    stagingBuffer.map_write(0, imageSize, data);
    stagingBuffer.unmap();

    mStagingBuffers.push_back(stagingBuffer);

    // image layout transition to transfer dst optimal
    RImageMemoryBarrier barrier{};
    barrier.image = dst;
    barrier.oldLayout = RIMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = RIMAGE_LAYOUT_TRANSFER_DST;
    barrier.srcAccess = 0;
    barrier.dstAccess = RACCESS_TRANSFER_WRITE_BIT;
    mList.cmd_image_memory_barrier(RPIPELINE_STAGE_TOP_OF_PIPE_BIT, RPIPELINE_STAGE_TRANSFER_BIT, barrier);

    // issue a full copy
    RBufferImageCopy region{
        .bufferOffset = 0,
        .imageWidth = dst.width(),
        .imageHeight = dst.height(),
        .imageDepth = dst.depth(),
        .imageLayers = dst.layers(),
    };
    mList.cmd_copy_buffer_to_image(stagingBuffer, dst, RIMAGE_LAYOUT_TRANSFER_DST, 1, &region);

    // image layout transition from transfer dst optimal to user requested final layout
    barrier.oldLayout = RIMAGE_LAYOUT_TRANSFER_DST;
    barrier.newLayout = finalLayout;
    barrier.srcAccess = RACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccess = 0;
    mList.cmd_image_memory_barrier(RPIPELINE_STAGE_TRANSFER_BIT, RPIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, barrier);
}

void RStager::submit(RQueue transferQueue)
{
    LD_PROFILE_SCOPE;

    mList.end();

    RSubmitInfo submitI{};
    submitI.listCount = 1;
    submitI.lists = &mList;
    transferQueue.submit(submitI, {});
    transferQueue.wait_idle();
}

} // namespace LD