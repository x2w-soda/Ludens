#include <iostream>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include "Core/Math/Include/Hex.h"
#include "Core/Media/Include/Model.h"
#include "Core/Media/Lib/ModelOBJ.h"
#include "Core/IO/Include/FileSystem.h"

namespace LD
{

struct TinyObjContext
{
    Model* Target = nullptr;
    std::string DirectoryPath;
    std::string FilePath;
    tinyobj::attrib_t Attrib;
    std::vector<tinyobj::shape_t> Shapes;
    std::vector<tinyobj::material_t> Materials;
    std::unordered_map<int, int> MaterialRefMap;
    int FallbackMaterialIdx;

    void ParseModel();
    void ParseShape(int obj_shape_idx);
    int ParseMtl(int obj_shape_idx, int obj_mat_id);
    int FallbackMtl(int obj_shape_idx);
    Ref<Image> ImportTexture(const Path& path);
};

void TinyObjContext::ParseModel()
{
    std::string warn, err;

    bool triangulate = true;
    bool success = tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &warn, &err, FilePath.c_str(), DirectoryPath.c_str(),
                                    triangulate);

    if (!warn.empty())
        std::cout << warn << std::endl;

    if (!err.empty())
        std::cout << err << std::endl;

    LD_DEBUG_ASSERT(success);

    Model& model = *Target;
    model.Materials.Clear();
    model.Meshes.Resize(Shapes.size());

    for (int shapeIdx = 0; shapeIdx < (int)Shapes.size(); shapeIdx++)
    {
        const tinyobj::shape_t& shape = Shapes[shapeIdx];
        LD_DEBUG_ASSERT(shape.mesh.material_ids.size() * 3 == shape.mesh.indices.size() && "shape is not triangulated");

        ParseShape(shapeIdx);
    }
}

// convert each tinyobj::shape_t to one Model::Mesh
// - each Model::Mesh at most references one Model::Material
// - this assumes tinyobj::shape_t only uses single tinyobj::material_t
void TinyObjContext::ParseShape(int obj_shape_idx)
{
    const tinyobj::shape_t& obj_shape = Shapes[obj_shape_idx];
    auto& mesh = Target->Meshes[obj_shape_idx];
    Mesh& ld_mesh = mesh.first;
    ld_mesh.Indices.Clear();
    ld_mesh.Vertices.Clear();

    // sanity check
    LD_DEBUG_ASSERT(obj_shape.mesh.indices.size() % 3 == 0);
    LD_DEBUG_ASSERT(obj_shape.mesh.material_ids.size() > 0);
    int obj_mat_id = obj_shape.mesh.material_ids[0];
    for (int id : obj_shape.mesh.material_ids)
        LD_DEBUG_ASSERT(id == obj_mat_id);

    // resolve Material for this Mesh
    int ld_mat_id = ParseMtl(obj_shape_idx, obj_mat_id);
    Material& ld_mat = Target->Materials[ld_mat_id].first;

    int pointN = 0;
    MeshVertex point[3];
    Vec3 edge1, edge2;
    Vec2 dUV1, dUV2;

    for (size_t idx = 0; idx < obj_shape.mesh.indices.size(); idx++, pointN = (pointN + 1) % 3)
    {
        tinyobj::index_t index = obj_shape.mesh.indices[idx];

        MeshVertex& vertex = point[pointN];
        vertex.Normal = Vec3::Zero;
        vertex.TexUV = Vec2::Zero;

        vertex.Position = {
            Attrib.vertices[3 * index.vertex_index + 0],
            Attrib.vertices[3 * index.vertex_index + 1],
            Attrib.vertices[3 * index.vertex_index + 2],
        };

        if (index.normal_index >= 0)
        {
            vertex.Normal = {
                Attrib.normals[3 * index.normal_index + 0],
                Attrib.normals[3 * index.normal_index + 1],
                Attrib.normals[3 * index.normal_index + 2],
            };
        }

        if (index.texcoord_index >= 0)
        {
            vertex.TexUV = {
                Attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - Attrib.texcoords[2 * index.texcoord_index + 1],
            };
        }

        if (pointN < 2)
            continue;

        edge1 = point[1].Position - point[0].Position;
        edge2 = point[2].Position - point[0].Position;
        dUV1 = point[1].TexUV - point[0].TexUV;
        dUV2 = point[2].TexUV - point[0].TexUV;
        float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);

        // generate tangents, assuming TexUV and Position valid
        Vec3& tangent = point[2].Tangent;
        tangent.x = f * (dUV2.y * edge1.x - dUV1.y * edge2.x);
        tangent.y = f * (dUV2.y * edge1.y - dUV1.y * edge2.y);
        tangent.z = f * (dUV2.y * edge1.z - dUV1.y * edge2.z);
        tangent = tangent.Normalized();
        point[1].Tangent = point[2].Tangent;
        point[0].Tangent = point[2].Tangent;

        if (index.normal_index < 0)
        {
            // generate face normals for each face manually
            point[2].Normal = Vec3::Cross(edge1, edge2).Normalized();
            point[1].Normal = point[2].Normal;
            point[0].Normal = point[2].Normal;
        }

        for (int i = 0; i < 3; i++)
        {
            MeshIndex meshIndex = (MeshIndex)(ld_mesh.Vertices.Size());
            ld_mesh.Vertices.PushBack(point[i]);
            ld_mesh.Indices.PushBack(meshIndex);
        }
    }
}

// convert each tinyobj::material_t to Model::Material,
// returns an index into LD::Model::Materials
int TinyObjContext::ParseMtl(int obj_shape_idx, int obj_mat_id)
{
    auto& mesh = Target->Meshes[obj_shape_idx];
    int& ld_mat_ref = mesh.second;
    Mesh& ld_mesh = mesh.first;

    // the mesh does not reference any material, use the fallback material
    if (obj_mat_id < 0)
    {
        return FallbackMtl(obj_shape_idx);
    }

    if (MaterialRefMap.find(obj_mat_id) != MaterialRefMap.end())
    {
        ld_mat_ref = MaterialRefMap[obj_mat_id];
        Target->Materials[ld_mat_ref].second.PushBack(obj_shape_idx);
        return ld_mat_ref;
    }

    // import new material
    tinyobj::material_t& obj_mat = Materials[obj_mat_id];
    auto& ld_mats = Target->Materials;
    ld_mat_ref = ld_mats.Size();

    ld_mats.PushBack({});
    Material& ld_mat = ld_mats.Back().first;
    Vector<int>& meshRefs = ld_mats.Back().second;

    meshRefs = { (int)obj_shape_idx };

    MaterialRefMap[obj_mat_id] = ld_mat_ref;

    ld_mat.Albedo = { obj_mat.diffuse[0], obj_mat.diffuse[1], obj_mat.diffuse[2], 1.0f };
    ld_mat.AlbedoTexture = nullptr;
    ld_mat.NormalTexture = nullptr;
    ld_mat.RoughnessTexture = nullptr;
    ld_mat.MetallicTexture = nullptr;
    ld_mat.MetallicRoughnessTexture = nullptr;
    ld_mat.Roughness = 0.0f;
    ld_mat.Metallic = 0.0f;

    if (!obj_mat.diffuse_texname.empty())
        ld_mat.AlbedoTexture = ImportTexture(Path{ DirectoryPath + obj_mat.diffuse_texname });

    // some models still reference their normal texture as bump textures
    if (!obj_mat.bump_texname.empty())
        ld_mat.NormalTexture = ImportTexture(Path{ DirectoryPath + obj_mat.bump_texname });

    if (!obj_mat.normal_texname.empty())
        ld_mat.NormalTexture = ImportTexture(Path{ DirectoryPath + obj_mat.normal_texname });

    return ld_mat_ref;
}

int TinyObjContext::FallbackMtl(int obj_shape_idx)
{
    auto& mesh = Target->Meshes[obj_shape_idx];
    int& ld_mat_ref = mesh.second;
    Mesh& ld_mesh = mesh.first;

    if (FallbackMaterialIdx < 0)
    {
        FallbackMaterialIdx = (int)Target->Materials.Size();
        Target->Materials.PushBack({});

        Material& fallback = Target->Materials.Back().first;
        fallback = Material::GetDefault();

        Vector<int>& meshRefs = Target->Materials.Back().second;
        meshRefs.Clear();
    }

    ld_mat_ref = FallbackMaterialIdx;
    Vector<int>& meshRefs = Target->Materials[ld_mat_ref].second;
    meshRefs.PushBack(obj_shape_idx);

    return ld_mat_ref;
}

Ref<Image> TinyObjContext::ImportTexture(const Path& path)
{
    bool exists = File::Exists(path);

    LD_DEBUG_ASSERT(exists);
    if (!exists)
        return nullptr;

    ImageLoader loader;
    return loader.LoadImage(path);
}

void LoadModelOBJ(const Path& path, Model& model)
{
    const auto& fs_path = static_cast<const std::filesystem::path&>(path);

    TinyObjContext obj{};
    obj.Target = &model;
    obj.DirectoryPath = fs_path.parent_path().string() + "/";
    obj.FilePath = fs_path.string();
    obj.FallbackMaterialIdx = -1;
    obj.ParseModel();
}

} // namespace LD