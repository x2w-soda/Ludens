#include "RUtilInternal.h"
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/KTX.h>
#include <ktx.h>
#include <vulkan/vulkan.h>

namespace LD {

bool KTXTexture::create(const KTXTextureInfo& info, KTXTexture& texture)
{
    LD_PROFILE_SCOPE;

    texture.handle = nullptr;

    VkFormat format;
    RUtil::cast_format_vk(info.format, format);

    uint32_t layerSize = info.width * info.height * RUtil::get_format_texel_size(info.format);
    if (layerSize * info.layers != info.dataSize)
    {
        // TODO: diagnostics
        return false;
    }

    ktxTextureCreateInfo textureCI{};
    textureCI.vkFormat = (ktx_uint32_t)format;
    textureCI.numLevels = 1;
    textureCI.numLayers = info.layers;
    textureCI.numFaces = 1;
    textureCI.baseWidth = info.width;
    textureCI.baseHeight = info.height;
    textureCI.baseDepth = 1;
    textureCI.numDimensions = 2;
    textureCI.isArray = KTX_FALSE;
    textureCI.generateMipmaps = KTX_FALSE;
    KTX_error_code result = ktxTexture2_Create(&textureCI, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture.handle);
    if (result != KTX_SUCCESS)
        return false;

    texture.width = info.width;
    texture.height = info.height;
    texture.layers = info.layers;
    texture.format = info.format;

    ktx_uint32_t level = 0;
    ktx_uint32_t faceSlice = 0;
    for (ktx_uint32_t layer = 0; layer < info.layers; layer++)
    {
        const byte* layerData = (const byte*)info.data + layerSize * layer;
        result = ktxTexture_SetImageFromMemory(ktxTexture(texture.handle), level, layer, faceSlice, layerData, layerSize);

        if (result != KTX_SUCCESS)
        {
            // TODO: diagnostics
            ktxTexture2_Destroy(texture.handle);
            texture.handle = nullptr;
            return false;
        }
    }

    ktxBasisParams params{};
    params.structSize = sizeof(ktxBasisParams);
    params.uastc = KTX_TRUE;
    params.uastcFlags = KTX_PACK_UASTC_LEVEL_FASTER;
    params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
    params.threadCount = std::thread::hardware_concurrency() - 2;
    result = ktxTexture2_CompressBasisEx(texture.handle, &params);
    if (result != KTX_SUCCESS)
    {
        ktxTexture2_Destroy(texture.handle);
        texture.handle = nullptr;
        return false;
    }

    return true;
}

void KTXTexture::destroy(KTXTexture texture)
{
    LD_PROFILE_SCOPE;

    if (!texture.handle)
        return;

    ktxTexture2_Destroy(texture.handle);
}

bool KTXTexture::write_to_disk(const FS::Path& path)
{
    LD_PROFILE_SCOPE;

    if (!handle)
        return false;

    std::string pathString = path.string();
    KTX_error_code result = ktxTexture2_WriteToNamedFile(handle, pathString.c_str());

    return result == KTX_SUCCESS;
}

} // namespace LD