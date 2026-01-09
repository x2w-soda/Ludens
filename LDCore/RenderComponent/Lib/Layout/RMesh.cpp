#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>

namespace LD {

static void rmesh_load_primitive(RMeshPrimitive* rprims, uint32_t& primCount, MeshNode* root)
{
    if (!root)
        return;

    for (const MeshPrimitive& prim : root->primitives)
    {
        RMeshPrimitive& rprim = rprims[primCount++];
        rprim.indexCount = prim.indexCount;
        rprim.indexStart = prim.indexStart;
        rprim.matIndex = prim.matIndex;
    }

    for (MeshNode* child : root->children)
        rmesh_load_primitive(rprims, primCount, child);
}

void RMesh::create_from_media(RDevice device, RStager& stager, Model& model)
{
    LD_PROFILE_SCOPE;

    memset(this, 0, sizeof(RMesh));
    this->device = device;

    MeshVertex* vertexData = model.get_vertices(vertexCount);
    uint32_t* indexData = model.get_indices(indexCount);
    MeshMaterial* matData = model.get_materials(matCount);
    Bitmap* textureData = model.get_textures(textureCount);
    model.get_primitives(primCount, nullptr);

    // TODO: some sort of allocator scheme
    textures = (RImage*)heap_malloc(sizeof(RImage) * textureCount, MEMORY_USAGE_RENDER);
    mats = (RMaterial*)heap_malloc(sizeof(RMaterial) * matCount, MEMORY_USAGE_RENDER);
    prims = (RMeshPrimitive*)heap_malloc(sizeof(RMeshPrimitive) * primCount, MEMORY_USAGE_RENDER);
    dummyTexture = {};

    // flatten tree hierachy into an array
    std::vector<MeshPrimitive> primData(primCount);
    model.get_primitives(primCount, primData.data());

    for (uint32_t i = 0; i < primCount; i++)
    {
        prims[i].indexCount = primData[i].indexCount;
        prims[i].indexStart = primData[i].indexStart;
        prims[i].matIndex = primData[i].matIndex;
    }

    LD_ASSERT(primCount == primCount);

    upload(stager, textureCount, textureData, matCount, matData, vertexCount, vertexData, indexCount, indexData);
}

void RMesh::create_from_binary(RDevice device, RStager& stager, ModelBinary& bin)
{
    LD_PROFILE_SCOPE;

    memset(this, 0, sizeof(RMesh));
    this->device = device;

    vertexCount = (uint32_t)bin.vertices.size();
    indexCount = (uint32_t)bin.indices.size();
    textureCount = (uint32_t)bin.textures.size();
    matCount = (uint32_t)bin.mats.size();
    primCount = (uint32_t)bin.prims.size();

    // TODO: some sort of allocator scheme
    textures = (RImage*)heap_malloc(sizeof(RImage) * textureCount, MEMORY_USAGE_RENDER);
    mats = (RMaterial*)heap_malloc(sizeof(RMaterial) * matCount, MEMORY_USAGE_RENDER);
    prims = (RMeshPrimitive*)heap_malloc(sizeof(RMeshPrimitive) * primCount, MEMORY_USAGE_RENDER);
    dummyTexture = {};

    for (uint32_t i = 0; i < primCount; i++)
    {
        prims[i].indexCount = bin.prims[i].indexCount;
        prims[i].indexStart = bin.prims[i].indexStart;
        prims[i].matIndex = bin.prims[i].matIndex;
    }

    upload(stager, textureCount, bin.textures.data(), matCount, bin.mats.data(), vertexCount, bin.vertices.data(), indexCount, bin.indices.data());
}

void RMesh::destroy()
{
    LD_PROFILE_SCOPE;

    device.destroy_buffer(ibo);
    device.destroy_buffer(vbo);
    device.destroy_set_pool(setPool);

    for (uint32_t i = 0; i < matCount; i++)
        device.destroy_buffer(mats[i].ubo);

    for (int i = 0; i < textureCount; i++)
        device.destroy_image(textures[i]);

    if (dummyTexture)
        device.destroy_image(dummyTexture);

    heap_free(mats);
    heap_free(textures);
    heap_free(prims);

    device = {};
}

void RMesh::upload(RStager& stager, uint32_t textureCount, const Bitmap* textureData,
                   uint32_t matCount, const MeshMaterial* matData,
                   uint32_t vertexCount, const MeshVertex* vertexData,
                   uint32_t indexCount, const uint32_t* indexData)
{
    // one set of bindings for each material
    RSetPoolInfo poolI{};
    poolI.maxSets = matCount;
    poolI.layout = sMaterialSetLayout;
    setPool = device.create_set_pool(poolI);

    std::vector<RSetBufferUpdateInfo> setBufferUpdates;
    std::vector<RSetImageUpdateInfo> setImageUpdates;

    // all image bindings should be initialized,
    // if material does not use textures, we still
    // have to use a dummy texture.
    bool useDummyTexture = textureCount == 0;
    for (uint32_t i = 0; i < matCount && !useDummyTexture; i++)
    {
        const MeshMaterial& mat = matData[i];

        if (mat.baseColorTextureIndex < 0 || mat.metallicRoughnessTextureIndex < 0 || mat.normalTextureIndex < 0)
            useDummyTexture = true;
    }

    if (useDummyTexture)
    {
        RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, 1, 1);
        dummyTexture = device.create_image(imageI);
        uint32_t whitePixel = 0xFFFFFFFF;
        stager.add_image_data(dummyTexture, &whitePixel, RIMAGE_LAYOUT_SHADER_READ_ONLY);
    }

    for (uint32_t i = 0; i < textureCount; i++)
    {
        Bitmap bitmap = textureData[i];
        RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT,
                                                      RFORMAT_RGBA8,
                                                      bitmap.width(), bitmap.height(),
                                                      {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_REPEAT});

        textures[i] = device.create_image(imageI);
        stager.add_image_data(textures[i], bitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    }

    for (uint32_t i = 0; i < matCount; i++)
    {
        const MeshMaterial& mat = matData[i];
        mats[i].set = setPool.allocate();
        mats[i].ubo = device.create_buffer({RBUFFER_USAGE_UNIFORM_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT, sizeof(RMaterialUBO), false});

        RMaterialUBO ubo;
        ubo.colorFactor = mat.baseColorFactor;
        ubo.metallicFactor = mat.metallicFactor;
        ubo.roughnessFactor = mat.roughnessFactor;
        ubo.hasColorTexture = mat.baseColorTextureIndex >= 0;
        ubo.hasNormalTexture = mat.normalTextureIndex >= 0;
        ubo.hasMetallicRoughnessTexture = mat.metallicRoughnessTextureIndex >= 0;
        stager.add_buffer_data(mats[i].ubo, &ubo);

        RImageLayout imageLayout = RIMAGE_LAYOUT_SHADER_READ_ONLY;
        setBufferUpdates.push_back(RUtil::make_single_set_buffer_udpate_info(mats[i].set, 0, RBINDING_TYPE_UNIFORM_BUFFER, &mats[i].ubo));

        if (ubo.hasColorTexture)
        {
            RImage* colorTexture = textures + mat.baseColorTextureIndex;
            setImageUpdates.push_back(RUtil::make_single_set_image_update_info(mats[i].set, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, colorTexture));
        }
        else
        {
            LD_ASSERT(dummyTexture);
            setImageUpdates.push_back(RUtil::make_single_set_image_update_info(mats[i].set, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, &dummyTexture));
        }

        if (ubo.hasNormalTexture)
        {
            RImage* normalTexture = textures + mat.normalTextureIndex;
            setImageUpdates.push_back(RUtil::make_single_set_image_update_info(mats[i].set, 2, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, normalTexture));
        }
        else
        {
            LD_ASSERT(dummyTexture);
            setImageUpdates.push_back(RUtil::make_single_set_image_update_info(mats[i].set, 2, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, &dummyTexture));
        }

        if (ubo.hasMetallicRoughnessTexture)
        {
            RImage* metallicRoughnessTexture = textures + mat.metallicRoughnessTextureIndex;
            setImageUpdates.push_back(RUtil::make_single_set_image_update_info(mats[i].set, 3, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, metallicRoughnessTexture));
        }
        else
        {
            LD_ASSERT(dummyTexture);
            setImageUpdates.push_back(RUtil::make_single_set_image_update_info(mats[i].set, 3, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &imageLayout, &dummyTexture));
        }
    }

    device.update_set_buffers((uint32_t)setBufferUpdates.size(), setBufferUpdates.data());
    device.update_set_images((uint32_t)setImageUpdates.size(), setImageUpdates.data());

    vbo = device.create_buffer({.usage = RBUFFER_USAGE_VERTEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT,
                                .size = sizeof(MeshVertex) * vertexCount,
                                .hostVisible = false});

    stager.add_buffer_data(vbo, vertexData);

    ibo = device.create_buffer({.usage = RBUFFER_USAGE_INDEX_BIT | RBUFFER_USAGE_TRANSFER_DST_BIT,
                                .size = sizeof(uint32_t) * indexCount,
                                .hostVisible = false});

    stager.add_buffer_data(ibo, indexData);
}

} // namespace LD