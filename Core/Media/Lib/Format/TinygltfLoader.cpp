#include "TinygltfLoader.h"
#include "../ModelObj.h"
#include <Ludens/Media/Model.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <iostream>
#include <string>

namespace LD {

bool TinygltfLoader::load_from_file(ModelObj* obj, const char* path)
{
    mObj = obj;

    std::string err, warn;
    bool result;
    
    {
        LD_PROFILE_SCOPE_NAME("Tinygltf::LoadASCIIFromFile");
        result = mContext.LoadASCIIFromFile(&mTinyModel, &err, &warn, path);
    }

    if (!warn.empty())
        std::cout << "load_gltf_model:warn: " << warn << std::endl;

    if (!err.empty())
        std::cout << "load_gltf_model:err: " << err << std::endl;

    if (!result)
        return false;

    load_model();

    return result;
}

bool TinygltfLoader::load_model()
{
    LD_PROFILE_SCOPE;

    if (!load_images())
        return false;

    if (!load_materials())
        return false;

    const tinygltf::Scene& tinyScene = mTinyModel.scenes[mTinyModel.defaultScene >= 0 ? mTinyModel.defaultScene : 0];

    mVertexCount = 0;
    mIndexCount = 0;

    for (size_t i = 0; i < tinyScene.nodes.size(); i++)
        scan_node_primitives(mTinyModel.nodes[tinyScene.nodes[i]]);

    std::cout << "TinygltfLoader: " << mVertexCount << " vertices" << std::endl;
    std::cout << "TinygltfLoader: " << mIndexCount << " indices" << std::endl;

    mObj->vertices.resize(mVertexCount);
    mObj->indices.resize(mIndexCount);

    for (size_t i = 0; i < tinyScene.nodes.size(); i++)
    {
        uint32_t nodeIndex = (uint32_t)tinyScene.nodes[i];
        if (!load_node(mTinyModel.nodes[i], nodeIndex, nullptr))
            return false;
    }

    std::cout << "TinygltfLoader: " << mObj->nodes.size() << " nodes" << std::endl;

    return true;
}

bool TinygltfLoader::load_images()
{
    LD_PROFILE_SCOPE;

    mObj->textures.resize(mTinyModel.images.size());

    std::cout << "TinygltfLoader: " << mObj->textures.size() << " textures" << std::endl;

    for (size_t i = 0; i < mObj->textures.size(); i++)
    {
        const tinygltf::Image& tinyImage = mTinyModel.images[i];

        if (tinyImage.component != 4)
            return false; // TODO:

        mObj->textures[i] = Bitmap::create_from_data(tinyImage.width, tinyImage.height, BITMAP_CHANNEL_RGBA, tinyImage.image.data());
    }

    return true;
}

bool TinygltfLoader::load_materials()
{
    LD_PROFILE_SCOPE;

    mObj->materials.resize(mTinyModel.materials.size());

    std::cout << "TinygltfLoader: " << mObj->materials.size() << " materials" << std::endl;

    for (size_t i = 0; i < mObj->materials.size(); i++)
    {
        tinygltf::Material& tinyMat = mTinyModel.materials[i];
        MeshMaterial& mat = mObj->materials[i];

        mat.baseColorFactor = {0.0f, 0.0f, 0.0f, 1.0f};
        mat.metallicFactor = 1.0f;
        mat.roughnessFactor = 1.0f;
        mat.baseColorTextureIndex = -1;
        mat.normalTextureIndex = -1;
        mat.metallicRoughnessTextureIndex = -1;

        if (tinyMat.values.contains("baseColorFactor"))
            mat.baseColorFactor = Vec4::from_data(tinyMat.values["baseColorFactor"].ColorFactor().data());

        if (tinyMat.values.contains("metallicFactor"))
            mat.metallicFactor = static_cast<float>(tinyMat.values["metallicFactor"].number_value);

        if (tinyMat.values.contains("roughnessFactor"))
            mat.roughnessFactor = static_cast<float>(tinyMat.values["roughnessFactor"].number_value);

        if (tinyMat.values.contains("baseColorTexture"))
        {
            int source = mTinyModel.textures[tinyMat.values["baseColorTexture"].TextureIndex()].source;
            int coordSet = tinyMat.values["baseColorTexture"].TextureTexCoord();
            if (coordSet != 0)
            {
                std::cout << "load_materials: base color texture uses coord set " << coordSet << std::endl;
                return false;
            }
            mat.baseColorTextureIndex = source;
        }

        if (tinyMat.additionalValues.contains("normalTexture"))
        {
            int source = mTinyModel.textures[tinyMat.additionalValues["normalTexture"].TextureIndex()].source;
            int coordSet = tinyMat.additionalValues["normalTexture"].TextureTexCoord();
            if (coordSet != 0)
            {
                std::cout << "load_materials: normal texture uses coord set " << coordSet << std::endl;
                return false;
            }
            mat.normalTextureIndex = source;
        }

        if (tinyMat.values.contains("metallicRoughnessTexture"))
        {
            int source = mTinyModel.textures[tinyMat.values["metallicRoughnessTexture"].TextureIndex()].source;
            int coordSet = tinyMat.values["metallicRoughnessTexture"].TextureTexCoord();
            if (coordSet != 0)
            {
                std::cout << "load_materials: metallic roughness texture uses coord set " << coordSet << std::endl;
                return false;
            }
            mat.metallicRoughnessTextureIndex = source;
        }
    }

    return true;
}

bool TinygltfLoader::load_node(tinygltf::Node& tinyNode, uint32_t nodeIndex, MeshNode* parent)
{
    MeshNode* node = heap_new<MeshNode>(MEMORY_USAGE_MEDIA); // TODO:
    node->name = tinyNode.name;
    node->parent = parent;
    Vec3 translation = {};
    Vec3 scale = Vec3(1.0f);
    Quat rotation = {};

    mObj->nodes.push_back(node);

    if (tinyNode.translation.size() == 3)
        translation = Vec3::from_data(tinyNode.translation.data());

    if (tinyNode.scale.size() == 3)
        scale = Vec3::from_data(tinyNode.scale.data());

    if (tinyNode.rotation.size() == 4)
        rotation = Quat::from_data(tinyNode.rotation.data());

    if (tinyNode.matrix.size() == 16)
    {
        Mat4 transform;
        transform[0] = Vec4::from_data(tinyNode.matrix.data());
        transform[1] = Vec4::from_data(tinyNode.matrix.data() + 4);
        transform[2] = Vec4::from_data(tinyNode.matrix.data() + 8);
        transform[3] = Vec4::from_data(tinyNode.matrix.data() + 12);
        node->localTransform = transform;
    }
    else
    {
        Mat4 T = Mat4::translate(translation);
        Mat4 R = Mat4::from_quat(rotation);
        Mat4 S = Mat4::scale(scale);
        node->localTransform = T * R * S;
    }

    for (size_t i = 0; i < tinyNode.children.size(); i++)
    {
        uint32_t childIndex = (uint32_t)tinyNode.children[i];
        load_node(mTinyModel.nodes[childIndex], childIndex, node);
    }

    if (tinyNode.mesh >= 0)
    {
        if (!load_mesh(mTinyModel.meshes[tinyNode.mesh], node))
            return false;
    }

    if (parent)
        parent->children.push_back(node);
    else
        mObj->roots.push_back(node);

    return true;
}

bool TinygltfLoader::load_mesh(tinygltf::Mesh& tinyMesh, MeshNode* node)
{
    node->primitives.resize(tinyMesh.primitives.size());

    for (size_t i = 0; i < node->primitives.size(); i++)
    {
        tinygltf::Primitive& tinyPrim = tinyMesh.primitives[i];
        MeshPrimitive& prim = node->primitives[i];

        uint32_t vertexBase = mVertexBase;
        uint32_t indexBase = mIndexBase;
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;
        const float* posBuffer = nullptr;
        const float* normalBuffer = nullptr;
        const float* uv0Buffer = nullptr;
        uint32_t posStride;
        uint32_t normalStride;
        uint32_t uv0Stride;

        if (tinyPrim.attributes.contains("POSITION"))
        {
            tinygltf::Accessor& posAcc = mTinyModel.accessors[tinyPrim.attributes["POSITION"]];
            tinygltf::BufferView& posView = mTinyModel.bufferViews[posAcc.bufferView];
            posBuffer = reinterpret_cast<const float*>(&(mTinyModel.buffers[posView.buffer].data[posAcc.byteOffset + posView.byteOffset]));
            posStride = (posAcc.ByteStride(posView) > 0) ? (posAcc.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
            vertexCount = (uint32_t)posAcc.count;
        }

        if (tinyPrim.attributes.contains("NORMAL"))
        {
            tinygltf::Accessor& normalAcc = mTinyModel.accessors[tinyPrim.attributes["NORMAL"]];
            tinygltf::BufferView& normalView = mTinyModel.bufferViews[normalAcc.bufferView];
            normalBuffer = reinterpret_cast<const float*>(&(mTinyModel.buffers[normalView.buffer].data[normalAcc.byteOffset + normalView.byteOffset]));
            normalStride = normalAcc.ByteStride(normalView) > 0 ? (normalAcc.ByteStride(normalView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
        }

        if (tinyPrim.attributes.contains("TEXCOORD_0"))
        {
            tinygltf::Accessor& uv0Acc = mTinyModel.accessors[tinyPrim.attributes["TEXCOORD_0"]];
            tinygltf::BufferView& uv0View = mTinyModel.bufferViews[uv0Acc.bufferView];
            uv0Buffer = reinterpret_cast<const float*>(&(mTinyModel.buffers[uv0View.buffer].data[uv0Acc.byteOffset + uv0View.byteOffset]));
            uv0Stride = uv0Acc.ByteStride(uv0View) ? (uv0Acc.ByteStride(uv0View) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
        }

        for (uint32_t v = 0; v < vertexCount; v++)
        {
            MeshVertex& vert = mObj->vertices[mVertexBase++];
            vert.pos = Vec3::from_data(posBuffer + v * posStride);

            if (normalBuffer)
                vert.normal = Vec3::from_data(normalBuffer + v * normalStride);

            if (uv0Buffer)
                vert.uv = Vec2::from_data(uv0Buffer + v * uv0Stride);
        }

        if (tinyPrim.indices >= 0)
        {
            tinygltf::Accessor& acc = mTinyModel.accessors[tinyPrim.indices];
            tinygltf::BufferView& bufferView = mTinyModel.bufferViews[acc.bufferView];
            tinygltf::Buffer& buffer = mTinyModel.buffers[bufferView.buffer];

            indexCount = (uint32_t)acc.count;
            const void* dataPtr = &(buffer.data[acc.byteOffset + bufferView.byteOffset]);

            switch (acc.componentType)
            {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = (const uint32_t*)dataPtr;
                for (uint32_t i = 0; i < indexCount; i++)
                    mObj->indices[mIndexBase++] = buf[i] + vertexBase;
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = (const uint16_t*)dataPtr;
                for (uint32_t i = 0; i < indexCount; i++)
                    mObj->indices[mIndexBase++] = static_cast<uint32_t>(buf[i] + vertexBase);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = (const uint8_t*)dataPtr;
                for (uint32_t i = 0; i < indexCount; i++)
                    mObj->indices[mIndexBase++] = static_cast<uint32_t>(buf[i] + vertexBase);
                break;
            }
            default:
                std::cout << "load_mesh: unsupported index bit depth";
                return false;
            }
        }

        prim.indexStart = indexBase;
        prim.indexCount = indexCount;
        prim.vertexStart = vertexBase;
        prim.vertexCount = vertexCount;
        prim.matIndex = (int32_t)tinyPrim.material;
    }

    return true;
}

void TinygltfLoader::scan_node_primitives(tinygltf::Node& tinyNode)
{
    if (tinyNode.children.size() > 0)
    {
        for (size_t i = 0; i < tinyNode.children.size(); i++)
            scan_node_primitives(mTinyModel.nodes[tinyNode.children[i]]);
    }

    if (tinyNode.mesh >= 0)
    {
        tinygltf::Mesh& tinyMesh = mTinyModel.meshes[tinyNode.mesh];

        for (size_t i = 0; i < tinyMesh.primitives.size(); i++)
        {
            tinygltf::Primitive& tinyPrim = tinyMesh.primitives[i];
            mVertexCount += mTinyModel.accessors[tinyPrim.attributes.find("POSITION")->second].count;

            if (tinyPrim.indices >= 0)
                mIndexCount += mTinyModel.accessors[tinyPrim.indices].count;
        }
    }
}

} // namespace LD