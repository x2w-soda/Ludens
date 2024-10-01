#pragma once

#include <cstddef>
#include <utility>
#include "Core/Math/Include/Hash.h"
#include "Core/Math/Include/Vec2.h"
#include "Core/Math/Include/Vec3.h"
#include "Core/DSA/Include/Vector.h"

namespace LD
{

struct MeshVertex
{
    Vec3 Position;
    Vec3 Normal;
    Vec3 Tangent;
    Vec2 TexUV;

    bool operator==(const MeshVertex& other) const
    {
        const char* l = (const char*)(this);
        const char* r = (const char*)(&other);

        for (size_t i = 0; i < sizeof(MeshVertex); i++)
            if (l[i] != r[i])
                return false;

        return true;
    }
};

using MeshIndex = u32;

struct Mesh
{
    Vector<MeshVertex> Vertices;
    Vector<MeshIndex> Indices;
};

void GenerateBoxMesh(Mesh& mesh, const Vec3& halfExtent);
void GenerateSphereMesh(Mesh& mesh, float radius, int stackCount, int sectorCount);

} // namespace LD

namespace std
{

template <>
struct hash<LD::Vec3>
{
    size_t operator()(LD::Vec3 const& vec) const
    {
        std::hash<float> hx, hy, hz;
        size_t result = 0;
        LD::HashCombine(result, hx(vec.x), hy(vec.y), hz(vec.z));
        return result;
    }
};

template <>
struct hash<LD::Vec2>
{
    size_t operator()(LD::Vec2 const& vec) const
    {
        std::hash<float> hx, hy;
        size_t result = 0;
        LD::HashCombine(result, hx(vec.x), hy(vec.y));
        return result;
    }
};

template <>
struct hash<LD::MeshVertex>
{
    size_t operator()(LD::MeshVertex const& vertex) const
    {
        hash<LD::Vec3> hp, hn;
        hash<LD::Vec2> huv;
        size_t result = 0;
        LD::HashCombine(result, hp(vertex.Position), hp(vertex.Normal), huv(vertex.TexUV));
        return result;
    }
};

} // namespace std