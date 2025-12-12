#include "RenderUtil.h"
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Pipeline/EquirectangularPipeline.h>
#include <Ludens/System/Memory.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("LDBuilder");

struct RenderUtilObj
{
    RDevice device;
    RCommandPool cmdPool;
    RCommandList list;
};

RenderUtil LD::RenderUtil::create()
{
    RenderUtilObj* obj = heap_new<RenderUtilObj>(MEMORY_USAGE_MISC);

    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    deviceI.window = nullptr;

    obj->device = RDevice::create(deviceI);
    obj->cmdPool = obj->device.create_command_pool({RQUEUE_TYPE_GRAPHICS, true});
    obj->list = obj->cmdPool.allocate();

    return {obj};
}

void RenderUtil::destroy(RenderUtil util)
{
    RenderUtilObj* obj = util;

    obj->device.destroy_command_pool(obj->cmdPool);
    RDevice::destroy(obj->device);

    heap_delete<RenderUtilObj>(obj);
}

void RenderUtil::from_equirectangular_to_faces(const fs::path& path, const fs::path& dstDirectory)
{
    RDevice device = mObj->device;

    Bitmap tmpBitmap = Bitmap::create_from_path(path.string().c_str(), true);
    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT,
                                                  RFORMAT_RGBA32F, tmpBitmap.width(), tmpBitmap.height(),
                                                  {.filter = RFILTER_LINEAR, .mipmapFilter = RFILTER_LINEAR, .addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE});

    RImage eqImage = device.create_image(imageI);
    EquirectangularPipeline eqPipeline = EquirectangularPipeline::create(device);

    // equirectangular images have an aspect ratio of 2:1,
    // and the width decides the cubeface resolution.
    // eg. we sample 2048x1024 equirectangular images for 2K cubemap images
    uint32_t faceSize = tmpBitmap.width();

    // TODO: parameterize faceSize
    faceSize = std::min<uint32_t>(faceSize, 2048);

    RBuffer faceBuffers[6];
    RImage faceImages[6];
    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(eqImage, tmpBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    stager.submit(device.get_graphics_queue());
    Bitmap::destroy(tmpBitmap);

    sLog.info("sampling {}x{} equirectangular texture: {}", eqImage.width(), eqImage.height(), path.string());

    for (int i = 0; i < 6; i++)
    {
        imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_COLOR_ATTACHMENT_BIT | RIMAGE_USAGE_TRANSFER_SRC_BIT, RFORMAT_RGBA8, faceSize, faceSize);
        faceImages[i] = device.create_image(imageI);
        faceBuffers[i] = device.create_buffer({RBUFFER_USAGE_TRANSFER_DST_BIT, faceImages[i].size(), true});
    }

    equirectangular_cmd_render_to_faces(device, eqPipeline, eqImage, faceImages, faceBuffers);

    sLog.info("sampling complete, beging writing to 6 faces");

    // save 6 bitmaps to disk concurrently
    struct Job
    {
        JobHeader header;
        RBuffer faceBuffer;
        uint32_t faceSize = 0;
        fs::path savePath;

        Job()
        {
            header.fn = &Job::job_main;
            header.user = this;
        }

        static void job_main(void* user)
        {
            Job* job = (Job*)user;
            job->faceBuffer.map();

            void* pixels = job->faceBuffer.map_read(0, job->faceBuffer.size());
            BitmapView view{
                .width = job->faceSize,
                .height = job->faceSize,
                .format = BITMAP_FORMAT_RGBA8U,
                .data = (const char*)pixels,
            };

            Bitmap::save_to_disk(view, job->savePath.string().c_str());
            job->faceBuffer.unmap();
        }
    };

    const char* fileNames[6] = {
        "px.png",
        "nx.png",
        "py.png",
        "ny.png",
        "pz.png",
        "nz.png",
    };

    Job jobs[6];
    JobSystem js = JobSystem::get();

    for (int i = 0; i < 6; i++)
    {
        jobs[i].faceBuffer = faceBuffers[i];
        jobs[i].faceSize = faceSize;
        jobs[i].savePath = fs::path(dstDirectory).append(fileNames[i]);
        js.submit((JobHeader*)(jobs + i), JOB_DISPATCH_IMMEDIATE);
    }

    js.wait_all();

    // RDevice is not thread safe (yet), resource destruction is not part of Job
    for (int i = 0; i < 6; i++)
    {
        device.destroy_buffer(faceBuffers[i]);
        device.destroy_image(faceImages[i]);
    }

    EquirectangularPipeline::destroy(eqPipeline);
    device.destroy_image(eqImage);
}

} // namespace LD
