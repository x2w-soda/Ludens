#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderFX/Include/RMesh.h"
#include "Core/Media/Include/Image.h"

namespace LD
{

RMesh::RMesh()
{
    mDevice.ResetHandle();
}

RMesh::~RMesh()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void RMesh::Startup(const RMeshInfo& info)
{
    const Model& model = *info.Data;
    mDevice = info.Device;

    LD_DEBUG_ASSERT(mDevice);

    size_t materialCount = model.Materials.Size();
    mBatches.Resize(materialCount);

    for (size_t batchIdx = 0; batchIdx < mBatches.Size(); batchIdx++)
    {
        const Material& mat = model.Materials[batchIdx].first;
        const Vector<int>& meshRefs = model.Materials[batchIdx].second;
        Batch& batch = mBatches[batchIdx];
        MaterialGroup& matBG = batch.Material;
        MaterialGroupInfo matBGI;
        matBGI.Device = mDevice;
        matBGI.MaterialBGL = info.MaterialBGL;
        matBGI.UBO.Albedo = mat.Albedo;
        matBGI.UBO.UseAlbedoTexture = 0;
        matBGI.UBO.UseNormalTexture = 0;
        matBGI.UBO.Roughness = mat.Roughness;
        matBGI.UBO.Metallic = mat.Metallic;

        if (mat.AlbedoTexture)
        {
            RTextureInfo info{};
            info.Type = RTextureType::Texture2D;
            info.Format = RTextureFormat::RGBA8;
            info.Width = (u32)mat.AlbedoTexture->GetWidth();
            info.Height = (u32)mat.AlbedoTexture->GetHeight();
            info.Data = (const void*)mat.AlbedoTexture->Pixels();
            info.Size = mat.AlbedoTexture->ByteSize();
            info.Sampler.MagFilter = RSamplerFilter::Linear;
            info.Sampler.MinFilter = RSamplerFilter::Linear;
            info.Sampler.AddressMode = RSamplerAddressMode::Repeat;

            matBGI.AlbedoTextureInfo = info;
            matBGI.UBO.UseAlbedoTexture = 1.0f;
        }

        if (mat.NormalTexture)
        {
            RTextureInfo info{};
            info.Type = RTextureType::Texture2D;
            info.Format = RTextureFormat::RGBA8;
            info.Width = (u32)mat.NormalTexture->GetWidth();
            info.Height = (u32)mat.NormalTexture->GetHeight();
            info.Data = mat.NormalTexture->Pixels();
            info.Size = mat.NormalTexture->ByteSize();
            info.Sampler.MagFilter = RSamplerFilter::Linear;
            info.Sampler.MinFilter = RSamplerFilter::Linear;
            info.Sampler.AddressMode = RSamplerAddressMode::Repeat;

            matBGI.NormalTextureInfo = info;
            matBGI.UBO.UseNormalTexture = 1;
        }

        // PBR metallic roughness information can be stored in many different ways
        PrepareMetallicRoughnessInfo(matBGI, mat);

        matBG.Startup(matBGI);

        // batch all geometry that uses the current material
        Vector<MeshVertex> batchVertices;
        Vector<u32> batchIndices;
        batch.IndexCount = 0;
        batch.VertexCount = 0;
        u32 indexBase = 0;
        u32 vertexBase = 0;

        for (int meshIdx : meshRefs)
        {
            const Mesh& mesh = model.Meshes[meshIdx].first;
            int materialRef = model.Meshes[meshIdx].second;
            LD_DEBUG_ASSERT(materialRef == batchIdx);

            vertexBase = batchVertices.Size();
            batchVertices.Resize(vertexBase + mesh.Vertices.Size());

            for (size_t vertexIdx = 0; vertexIdx < mesh.Vertices.Size(); vertexIdx++)
            {
                batchVertices[vertexBase + vertexIdx] = mesh.Vertices[vertexIdx];
            }

            indexBase = batchIndices.Size();
            batchIndices.Resize(indexBase + mesh.Indices.Size());

            for (size_t indexIdx = 0; indexIdx < mesh.Indices.Size(); indexIdx++)
            {
                batchIndices[indexBase + indexIdx] = vertexBase + mesh.Indices[indexIdx];
            }

            batch.VertexCount += mesh.Vertices.Size();
            batch.IndexCount += mesh.Indices.Size();
        }

        RBufferInfo vboInfo{};
        vboInfo.Type = RBufferType::VertexBuffer;
        vboInfo.Data = batchVertices.Data();
        vboInfo.Size = batchVertices.ByteSize();
        mDevice.CreateBuffer(batch.Vertices, vboInfo);

        RBufferInfo iboInfo{};
        iboInfo.Type = RBufferType::IndexBuffer;
        iboInfo.Data = batchIndices.Data();
        iboInfo.Size = batchIndices.ByteSize();
        mDevice.CreateBuffer(batch.Indices, iboInfo);
    }
}

void RMesh::Cleanup()
{
    for (auto& batch : mBatches)
    {
        mDevice.DeleteBuffer(batch.Indices);
        mDevice.DeleteBuffer(batch.Vertices);
        batch.Material.Cleanup();
    }

    mDevice.ResetHandle();
}

void RMesh::Draw(BatchFn fn)
{
    LD_DEBUG_ASSERT(mDevice);

    for (auto& batch : mBatches)
    {
        fn(batch);
    }
}

void RMesh::PrepareMetallicRoughnessInfo(MaterialGroupInfo& matBGI, const Material& mat)
{
    RTextureInfo metallicI{};
    metallicI.Format = RTextureFormat::RGBA8;
    metallicI.Sampler.MagFilter = RSamplerFilter::Linear;
    metallicI.Sampler.MinFilter = RSamplerFilter::Linear;
    metallicI.Sampler.AddressMode = RSamplerAddressMode::Repeat;

    RTextureInfo roughnessI{};
    roughnessI.Format = RTextureFormat::RGBA8;
    roughnessI.Sampler.MagFilter = RSamplerFilter::Linear;
    roughnessI.Sampler.MinFilter = RSamplerFilter::Linear;
    roughnessI.Sampler.AddressMode = RSamplerAddressMode::Repeat;

    if (mat.MetallicRoughnessTexture)
    {
        metallicI.Width = (u32)mat.MetallicRoughnessTexture->GetWidth();
        metallicI.Height = (u32)mat.MetallicRoughnessTexture->GetHeight();
        metallicI.Data = mat.MetallicRoughnessTexture->Pixels();
        metallicI.Size = mat.MetallicRoughnessTexture->ByteSize();
        matBGI.MetallicTextureInfo = metallicI;
        matBGI.RoughnessTextureInfo.Reset();
        matBGI.MetallicRoughnessLayout = MetallicRoughnessInfo::SingleTexture;
    }
    else if (mat.MetallicTexture && mat.RoughnessTexture)
    {
        metallicI.Width = (u32)mat.MetallicTexture->GetWidth();
        metallicI.Height = (u32)mat.MetallicTexture->GetHeight();
        metallicI.Data = mat.MetallicTexture->Pixels();
        metallicI.Size = mat.MetallicTexture->ByteSize();
        roughnessI.Width = (u32)mat.RoughnessTexture->GetWidth();
        roughnessI.Height = (u32)mat.RoughnessTexture->GetHeight();
        roughnessI.Data = mat.RoughnessTexture->Pixels();
        roughnessI.Size = mat.RoughnessTexture->ByteSize();
        matBGI.MetallicTextureInfo = metallicI;
        matBGI.RoughnessTextureInfo = roughnessI;
        matBGI.MetallicRoughnessLayout = MetallicRoughnessInfo::SeparateTextures;
    }
    else if (mat.MetallicTexture && !mat.RoughnessTexture)
    {
        metallicI.Width = (u32)mat.MetallicTexture->GetWidth();
        metallicI.Height = (u32)mat.MetallicTexture->GetHeight();
        metallicI.Data = mat.MetallicTexture->Pixels();
        metallicI.Size = mat.MetallicTexture->ByteSize();
        matBGI.MetallicTextureInfo = metallicI;
        matBGI.RoughnessTextureInfo.Reset();
        matBGI.MetallicRoughnessLayout = MetallicRoughnessInfo::MetallicTextureOnly;
    }
    else if (!mat.MetallicTexture && mat.RoughnessTexture)
    {
        roughnessI.Width = (u32)mat.RoughnessTexture->GetWidth();
        roughnessI.Height = (u32)mat.RoughnessTexture->GetHeight();
        roughnessI.Data = mat.RoughnessTexture->Pixels();
        roughnessI.Size = mat.RoughnessTexture->ByteSize();
        matBGI.MetallicTextureInfo.Reset();
        matBGI.RoughnessTextureInfo = roughnessI;
        matBGI.MetallicRoughnessLayout = MetallicRoughnessInfo::RoughnessTextureOnly;
    }
    else
        matBGI.MetallicRoughnessLayout = MetallicRoughnessInfo::None;

    // don't forget to make this flag visible from the shader in the UBO
    matBGI.UBO.MetallicRoughnessLayout = (i32)matBGI.MetallicRoughnessLayout;
}

} // namespace LD