#include <iostream>
#include <tinygltf/tiny_gltf.h>
#include "Core/IO/Include/FileSystem.h"
#include "Core/DSA/Include/String.h"
#include "Core/DSA/Include/View.h"
#include "Core/Media/Include/Model.h"
#include "Core/Media/Include/Image.h"
#include "Core/Media/Lib/ModelGLTF.h"

namespace LD
{

static void Dump(const tinygltf::Model& model, String& str);
static void DumpPrimitive(const tinygltf::Primitive& prim, String& str, int indent);
static void DumpAccessor(const tinygltf::Accessor& acc, String& str, int indent);
static void DumpMaterial(const tinygltf::Material& mat, String& str, int indent);
static void DumpImage(const tinygltf::Image& image, String& str, int indent);
static void DumpTexture(const tinygltf::Texture& texture, String& str, int indent);
static void DumpTextureInfo(const tinygltf::TextureInfo& info, String& str, int indent);
static void DumpNormalTextureInfo(const tinygltf::NormalTextureInfo& info, String& str, int indent);
static void DumpOcclusionTextureInfo(const tinygltf::OcclusionTextureInfo& info, String& str, int indent);
static void DumpPBRMetallicRoughness(const tinygltf::PbrMetallicRoughness& pbr, String& str, int indent);

static String PrimitiveMode(int mode)
{
    switch (mode)
    {
    case TINYGLTF_MODE_POINTS:
        return "POINTS";
    case TINYGLTF_MODE_LINE:
        return "LINE";
    case TINYGLTF_MODE_LINE_LOOP:
        return "LINE_LOOP";
    case TINYGLTF_MODE_TRIANGLES:
        return "TRIANGLES";
    case TINYGLTF_MODE_TRIANGLE_FAN:
        return "TRIANGLE_FAN";
    case TINYGLTF_MODE_TRIANGLE_STRIP:
        return "TRIANGLE_STRIP";
    default:
        break;
    }
    LD_DEBUG_UNREACHABLE;
}

static String GLTFType(int type)
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR:
        return "SCALAR";
    case TINYGLTF_TYPE_VECTOR:
        return "VECTOR";
    case TINYGLTF_TYPE_VEC2:
        return "VEC2";
    case TINYGLTF_TYPE_VEC3:
        return "VEC3";
    case TINYGLTF_TYPE_VEC4:
        return "VEC4";
    case TINYGLTF_TYPE_MATRIX:
        return "MATRIX";
    case TINYGLTF_TYPE_MAT2:
        return "MAT2";
    case TINYGLTF_TYPE_MAT3:
        return "MAT3";
    case TINYGLTF_TYPE_MAT4:
        return "MAT4";
    default:
        break;
    }
    LD_DEBUG_UNREACHABLE;
}

static size_t GLTFComponentByteSize(int component)
{
    switch (component)
    {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return 1;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return 2;
    case TINYGLTF_COMPONENT_TYPE_INT:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return 4;
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        return 8;
    default:
        break;
    }
    LD_DEBUG_UNREACHABLE;
}

static inline String Indent(int indent)
{
    return String((size_t)indent, ' ');
}

struct TinyGLTFContext
{
    tinygltf::TinyGLTF Parser;
    tinygltf::Model GLTF;

    void* AccessData(const tinygltf::Accessor& acc, size_t& offset, size_t& len)
    {
        const tinygltf::BufferView& view = GLTF.bufferViews[acc.bufferView];
        offset = view.byteOffset + acc.byteOffset;
        len = view.byteLength;

        Byte* data = (Byte*)(GLTF.buffers[view.buffer].data.data()) + offset;

        return (void*)data;
    }

    template <typename T>
    T* AccessDataType(const tinygltf::Accessor& acc, size_t& offset, size_t& len, size_t& stride)
    {
        const tinygltf::BufferView& view = GLTF.bufferViews[acc.bufferView];
        offset = view.byteOffset + acc.byteOffset;
        len = view.byteLength;
        stride = view.byteStride;

        Byte* data = (Byte*)(GLTF.buffers[view.buffer].data.data()) + offset;

        return (T*)data;
    }

    void ImportModel(Model& model);
    void ImportAttribute(const std::pair<std::string, int>& attr, LD::Mesh& ld_mesh, size_t vertexCount);
    Ref<Image> ImportTexture(int textureIndex);
};

void LoadModelGLTFAscii(const Path& path, Model& model)
{
    TinyGLTFContext ctx;

    std::string err, warn;

    bool ok = ctx.Parser.LoadASCIIFromFile(&ctx.GLTF, &err, &warn, path.ToString());

    if (!err.empty())
        std::cout << "TinyGLTF::LoadASCIIFromFile error: " << err << std::endl;

    if (!warn.empty())
        std::cout << "TinyGLTF::LoadASCIIFromFile warning: " << warn << std::endl;

    // TODO: error handling
    LD_DEBUG_ASSERT(ok);
}

void LoadModelGLTFBinary(const Path& path, Model& model)
{
    TinyGLTFContext ctx;

    std::string err, warn;

    bool ok = ctx.Parser.LoadBinaryFromFile(&ctx.GLTF, &err, &warn, path.ToString());

    if (!err.empty())
        std::cout << "TinyGLTF::LoadBinaryFromFile error: " << err << std::endl;

    if (!warn.empty())
        std::cout << "TinyGLTF::LoadBinaryFromFile warning: " << warn << std::endl;

    // TODO: error handling
    LD_DEBUG_ASSERT(ok);

    ctx.ImportModel(model);
}

void Dump(const tinygltf::Model& model, String& str)
{
    str << "asset_copyright: " << model.asset.copyright << '\n';
    str << "asset_generator: " << model.asset.generator << '\n';
    str << "asset_version: " << model.asset.version << '\n';
    str << "asset_min_version: " << model.asset.minVersion << '\n';
    str << "default_scene: " << model.defaultScene << '\n';

    str << "scenes: " << model.scenes.size() << '\n';
    for (size_t i = 0; i < model.scenes.size(); i++)
    {
        str << Indent(1) << model.scenes[i].name << '\n';
    }

    str << "meshes: " << model.meshes.size() << '\n';
    for (size_t i = 0; i < model.meshes.size(); i++)
    {
        const tinygltf::Mesh& mesh = model.meshes[i];
        str << Indent(1) << mesh.name << '\n';

        for (size_t j = 0; j < mesh.primitives.size(); j++)
        {
            const tinygltf::Primitive& prim = mesh.primitives[j];
            DumpPrimitive(prim, str, 2);
        }
    }

    str << "accessors: " << model.accessors.size() << '\n';
    for (size_t i = 0; i < model.accessors.size(); i++)
    {
        const tinygltf::Accessor& acc = model.accessors[i];
        str << " accessor " << i << ":\n";
        DumpAccessor(acc, str, 2);
    }

    if (!model.animations.empty())
    {
        str << "animations: " << model.animations.size() << '\n';
        // TODO:
    }

    // TODO:
    str << "buffers: " << model.buffers.size() << '\n';

    // TODO:
    str << "buffer_views: " << model.bufferViews.size() << '\n';

    str << "materials: " << model.materials.size() << '\n';
    for (size_t i = 0; i < model.materials.size(); i++)
    {
        const tinygltf::Material& mat = model.materials[i];

        str << Indent(1) << mat.name << '\n';
        DumpMaterial(mat, str, 1);
    }

    if (!model.images.empty())
    {
        str << "images: " << model.images.size() << '\n';
        for (size_t i = 0; i < model.images.size(); i++)
            DumpImage(model.images[i], str, 1);
    }

    if (!model.textures.empty())
    {
        str << "textures: " << model.textures.size() << '\n';
        for (size_t i = 0; i < model.textures.size(); i++)
            DumpTexture(model.textures[i], str, 1);
    }
}

void DumpPrimitive(const tinygltf::Primitive& prim, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "material_ref: " << prim.material << '\n';
    str << ind << "indices: " << prim.indices << '\n';
    str << ind << "mode: " << PrimitiveMode(prim.mode) << '\n';
    str << ind << "attributes: " << prim.attributes.size() << '\n';

    for (auto attr : prim.attributes)
    {
        str << Indent(indent + 1) << attr.first << ": " << attr.second << '\n';
    }
}

void DumpAccessor(const tinygltf::Accessor& acc, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "name: " << acc.name << '\n';
    str << ind << "buffer_view: " << acc.bufferView << '\n';
    str << ind << "byte_offset: " << acc.byteOffset << '\n';
    str << ind << "count: " << acc.count << '\n';
    str << ind << "type: " << GLTFType(acc.type) << '\n';
}

void DumpMaterial(const tinygltf::Material& mat, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "alpha_mode: " << mat.alphaMode << '\n';
    str << ind << "alpha_cutoff: " << mat.alphaCutoff << '\n';

    str << ind << "emissive_factor:";
    LD_DEBUG_ASSERT(mat.emissiveFactor.size() == 3);
    for (int i = 0; i < 3; i++)
        str << ind << ' ' << mat.emissiveFactor[i];
    str << '\n';

    DumpPBRMetallicRoughness(mat.pbrMetallicRoughness, str, indent + 1);

    if (mat.normalTexture.index >= 0)
    {
        str << ind << "normal_texture:\n";
        DumpNormalTextureInfo(mat.normalTexture, str, indent + 1);
    }

    if (mat.occlusionTexture.index >= 0)
    {
        str << ind << "occlusion_texture:\n";
        DumpOcclusionTextureInfo(mat.occlusionTexture, str, indent + 1);
    }

    if (mat.emissiveTexture.index >= 0)
    {
        str << ind << "emissive_texture:\n";
        DumpTextureInfo(mat.emissiveTexture, str, indent + 1);
    }
}

void DumpImage(const tinygltf::Image& image, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "name: " << image.name << '\n';
    str << ind << "width:  " << image.width << '\n';
    str << ind << "height: " << image.height << '\n';
    str << ind << "component: " << image.component << '\n';
}

void DumpTexture(const tinygltf::Texture& texture, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "sampler: " << texture.sampler << '\n';
    str << ind << "source: " << texture.source << '\n';
}

void DumpTextureInfo(const tinygltf::TextureInfo& info, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "index: " << info.index << '\n';
    str << ind << "tex_coord: " << info.texCoord << '\n';
}

void DumpNormalTextureInfo(const tinygltf::NormalTextureInfo& info, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "index: " << info.index << '\n';
    str << ind << "tex_coord: " << info.texCoord << '\n';
    str << ind << "scale: " << info.scale << '\n';
}

void DumpOcclusionTextureInfo(const tinygltf::OcclusionTextureInfo& info, String& str, int indent)
{
    String ind = Indent(indent);
    str << ind << "index: " << info.index << '\n';
    str << ind << "tex_coord: " << info.texCoord << '\n';
    str << ind << "strength: " << info.strength << '\n';
}

void DumpPBRMetallicRoughness(const tinygltf::PbrMetallicRoughness& pbr, String& str, int indent)
{
    String ind = Indent(indent);

    str << ind << "base_color_factor:";
    LD_DEBUG_ASSERT(pbr.baseColorFactor.size() == 4);
    for (int i = 0; i < 4; i++)
        str << ind << ' ' << pbr.baseColorFactor[i];
    str << '\n';

    if (pbr.baseColorTexture.index >= 0)
    {
        str << ind << "base_color_texture:\n";
        DumpTextureInfo(pbr.baseColorTexture, str, indent + 1);
    }

    str << ind << "metallic_factor: " << pbr.metallicFactor << '\n';
    str << ind << "roughness_factor: " << pbr.roughnessFactor << '\n';

    if (pbr.metallicRoughnessTexture.index >= 0)
    {
        str << ind << "metallic_roughness_texture:\n";
        DumpTextureInfo(pbr.metallicRoughnessTexture, str, indent + 1);
    }
}

void TinyGLTFContext::ImportModel(Model& model)
{
    // import materials
    // import each tinyobj::Material as a LD::Material
    model.Materials.Resize(GLTF.materials.size());

    for (size_t i = 0; i < GLTF.materials.size(); i++)
    {
        const tinygltf::Material& mat = GLTF.materials[i];
        const tinygltf::PbrMetallicRoughness& pbr = mat.pbrMetallicRoughness;

        model.Materials[i].second.Clear();
        LD::Material& ld_mat = model.Materials[i].first;

        const double* data = pbr.baseColorFactor.data();
        Vec4 albedo = { (float)data[0], (float)data[1], (float)data[2], (float)data[2] };

        ld_mat.Metallic = (float)pbr.metallicFactor;
        ld_mat.Roughness = (float)pbr.roughnessFactor;
        ld_mat.Albedo = albedo;
        ld_mat.AlbedoTexture = ImportTexture(pbr.baseColorTexture.index);
        ld_mat.NormalTexture = ImportTexture(mat.normalTexture.index);

        // NOTE: GLTF 2.0 cramps metallic and roughness into a single texture
        ld_mat.MetallicRoughnessTexture = ImportTexture(pbr.metallicRoughnessTexture.index);
        ld_mat.MetallicTexture = nullptr;
        ld_mat.RoughnessTexture = nullptr;
    }

    // import meshes
    // import each tinygltf::Primitive as a LD::Mesh
    size_t ld_mesh_count = 0;

    model.Meshes.Clear();

    for (const tinygltf::Mesh& mesh : GLTF.meshes)
    {
        for (const tinygltf::Primitive& prim : mesh.primitives)
        {
            LD_DEBUG_ASSERT(prim.mode == TINYGLTF_MODE_TRIANGLES);

            model.Meshes.PushBack({});
            model.Meshes.Back().second = prim.material;
            model.Materials[prim.material].second.PushBack(ld_mesh_count++);
            LD::Mesh& ld_mesh = model.Meshes.Back().first;

            // import mesh indices
            // convert indices to 32-bit if the model index is not 32-bit
            size_t dataOffset, dataSize, indexCount;
            const tinygltf::Accessor& acc = GLTF.accessors[prim.indices];

            void* indices = AccessData(acc, dataOffset, dataSize);
            size_t componentByteSize = GLTFComponentByteSize(acc.componentType);
            LD_DEBUG_ASSERT(dataSize % componentByteSize == 0);

            indexCount = dataSize / componentByteSize;
            ld_mesh.Indices.Resize(indexCount);

            switch (acc.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                for (size_t i = 0; i < indexCount; i++)
                    ld_mesh.Indices[i] = ((u16*)indices)[i];
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                for (size_t i = 0; i < indexCount; i++)
                    ld_mesh.Indices[i] = ((u32*)indices)[i];
                break;
            default:
                LD_DEBUG_UNREACHABLE;
            }

            // import mesh vertices
            // we need at least position and normal attributes
            size_t vertexCount;
            vertexCount = GLTF.accessors[prim.attributes.begin()->second].count;
            ld_mesh.Vertices.Resize(vertexCount);

            // TODO: calculate MeshVertex::Tangent per face
            for (size_t i = 0; i < vertexCount; i++)
                ld_mesh.Vertices[i].Tangent = Vec3::Zero;

            for (const auto& attr : prim.attributes)
            {
                ImportAttribute(attr, ld_mesh, vertexCount);
            }
        }
    }
}

void TinyGLTFContext::ImportAttribute(const std::pair<std::string, int>& attr, LD::Mesh& ld_mesh, size_t vertexCount)
{
    const tinygltf::Accessor& acc = GLTF.accessors[attr.second];
    size_t componentSize = GLTFComponentByteSize(acc.componentType);
    size_t dataOffset, dataSize, dataStride;

    LD_DEBUG_ASSERT(acc.count == vertexCount);

    // TODO: double
    LD_DEBUG_ASSERT(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

    if (attr.first == "POSITION")
    {
        Vec3* pos = AccessDataType<Vec3>(acc, dataOffset, dataSize, dataStride);
        if (dataStride < componentSize * 3)
            dataStride = componentSize * 3; // tightly packed

        for (size_t i = 0; i < acc.count; i++)
        {
            ld_mesh.Vertices[i].Position = *pos;
            pos = (Vec3*)((Byte*)pos + dataStride);
        }
    }
    else if (attr.first == "NORMAL")
    {
        Vec3* normal = AccessDataType<Vec3>(acc, dataOffset, dataSize, dataStride);
        if (dataStride < componentSize * 3)
            dataStride = componentSize * 3; // tightly packed

        for (size_t i = 0; i < acc.count; i++)
        {
            ld_mesh.Vertices[i].Normal = *normal;
            normal = (Vec3*)((Byte*)normal + dataStride);
        }
    }
    else if (attr.first == "TEXCOORD_0")
    {
        Vec2* uv = AccessDataType<Vec2>(acc, dataOffset, dataSize, dataStride);
        if (dataStride < componentSize * 2)
            dataStride = componentSize * 2; // tightly packed

        for (size_t i = 0; i < acc.count; i++)
        {
            ld_mesh.Vertices[i].TexUV = *uv;
            uv = (Vec2*)((Byte*)uv + dataStride);
        }
    }
    else
        LD_DEBUG_UNREACHABLE;
}

Ref<Image> TinyGLTFContext::ImportTexture(int textureIndex)
{
    if (textureIndex < 0)
        return nullptr;

    const tinygltf::Texture& texture = GLTF.textures[textureIndex];
    const tinygltf::Sampler& sampler = GLTF.samplers[texture.sampler];
    const tinygltf::Image& image = GLTF.images[texture.source];

    // TODO: texture is not embedded, import from disk
    LD_DEBUG_ASSERT(image.uri.empty());

    return MakeRef<Image>(image.width, image.height, 4, image.image.data());
}

} // namespace LD