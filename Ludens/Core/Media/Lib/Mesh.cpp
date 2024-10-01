#include "Core/Media/Include/Mesh.h"

namespace LD
{

void GenerateBoxMesh(Mesh& mesh, const Vec3& halfExtent)
{
    mesh.Vertices.Clear();
    mesh.Indices.Clear();

    int idxBase = 0;
    MeshVertex p0, p1, p2, p3;
    p0.TexUV = { 0.0f, 0.0f };
    p1.TexUV = { 0.0f, 0.0f };
    p2.TexUV = { 0.0f, 0.0f };
    p3.TexUV = { 0.0f, 0.0f };

    for (int sign = -1; sign <= 1; sign += 2)
    {
        // clang-format off

        p0.Position = { sign * -halfExtent.x, sign * halfExtent.y, -halfExtent.z };
        p1.Position = { sign * -halfExtent.x, sign * halfExtent.y,  halfExtent.z };
        p2.Position = { sign *  halfExtent.x, sign * halfExtent.y,  halfExtent.z };
        p3.Position = { sign *  halfExtent.x, sign * halfExtent.y, -halfExtent.z };
        p0.Normal = { 0.0f, (float)sign, 0.0f };
        p1.Normal = { 0.0f, (float)sign, 0.0f };
        p2.Normal = { 0.0f, (float)sign, 0.0f };
        p3.Normal = { 0.0f, (float)sign, 0.0f };

        mesh.Vertices.PushBack(p0);
        mesh.Vertices.PushBack(p1);
        mesh.Vertices.PushBack(p2);
        mesh.Vertices.PushBack(p3);

        mesh.Indices.PushBack(idxBase + 0);
        mesh.Indices.PushBack(idxBase + 1);
        mesh.Indices.PushBack(idxBase + 2);
        mesh.Indices.PushBack(idxBase + 2);
        mesh.Indices.PushBack(idxBase + 3);
        mesh.Indices.PushBack(idxBase + 0);
        idxBase += 4;

        p0.Position = { sign * halfExtent.x, sign *  halfExtent.y, halfExtent.z };
        p1.Position = { sign * halfExtent.x, sign * -halfExtent.y, halfExtent.z };
        p2.Position = { sign * halfExtent.x, sign * -halfExtent.y, -halfExtent.z };
        p3.Position = { sign * halfExtent.x, sign *  halfExtent.y, -halfExtent.z };
        p0.Normal = { (float)sign, 0.0f, 0.0f };
        p1.Normal = { (float)sign, 0.0f, 0.0f };
        p2.Normal = { (float)sign, 0.0f, 0.0f };
        p3.Normal = { (float)sign, 0.0f, 0.0f };

        mesh.Vertices.PushBack(p0);
        mesh.Vertices.PushBack(p1);
        mesh.Vertices.PushBack(p2);
        mesh.Vertices.PushBack(p3);

        mesh.Indices.PushBack(idxBase + 0);
        mesh.Indices.PushBack(idxBase + 1);
        mesh.Indices.PushBack(idxBase + 2);
        mesh.Indices.PushBack(idxBase + 2);
        mesh.Indices.PushBack(idxBase + 3);
        mesh.Indices.PushBack(idxBase + 0);
        idxBase += 4;

        p0.Position = { -halfExtent.x, sign *  halfExtent.y, sign * halfExtent.z };
        p1.Position = { -halfExtent.x, sign * -halfExtent.y, sign * halfExtent.z };
        p2.Position = {  halfExtent.x, sign * -halfExtent.y, sign * halfExtent.z };
        p3.Position = {  halfExtent.x, sign *  halfExtent.y, sign * halfExtent.z };
        p0.Normal = { 0.0f, 0.0f, (float)sign };
        p1.Normal = { 0.0f, 0.0f, (float)sign };
        p2.Normal = { 0.0f, 0.0f, (float)sign };
        p3.Normal = { 0.0f, 0.0f, (float)sign };

        mesh.Vertices.PushBack(p0);
        mesh.Vertices.PushBack(p1);
        mesh.Vertices.PushBack(p2);
        mesh.Vertices.PushBack(p3);

        mesh.Indices.PushBack(idxBase + 0);
        mesh.Indices.PushBack(idxBase + 1);
        mesh.Indices.PushBack(idxBase + 2);
        mesh.Indices.PushBack(idxBase + 2);
        mesh.Indices.PushBack(idxBase + 3);
        mesh.Indices.PushBack(idxBase + 0);
        idxBase += 4;

        // clang-format on
    }
}

void GenerateSphereMesh(Mesh& mesh, float radius, int stackCount, int sectorCount)
{
    mesh.Vertices.Clear();
    mesh.Indices.Clear();

    // implementation from https://www.songho.ca/opengl/gl_sphere.html

    float x, y, z, xz;
    float radiusInv = 1.0f / radius;

    Radians sectorStep = 2 * LD_MATH_PI / sectorCount;
    Radians stackStep = LD_MATH_PI / stackCount;
    Radians sectorAngle, stackAngle;

    // generate (sectorCount + 1) vertices for each stack
    for (int i = 0; i <= stackCount; i++)
    {
        // stack angle from PI/2 to -PI/2
        stackAngle = LD_MATH_PI / 2 - i * stackStep;
        xz = radius * LD_MATH_COS(stackAngle);
        y = radius * LD_MATH_SIN(stackAngle);

        for (int j = 0; j <= sectorCount; j++)
        {
            // sector angle from 0 to 2 PI
            sectorAngle = j * sectorStep;

            z = xz * LD_MATH_COS(sectorAngle);
            x = xz * LD_MATH_SIN(sectorAngle);

            MeshVertex& vertex = mesh.Vertices.PushBack();
            vertex.Position = { x, y, z };
            vertex.Normal = { x * radiusInv, y * radiusInv, z * radiusInv };
            vertex.TexUV = { j / (float)sectorCount, i / (float)stackCount };
            vertex.Tangent = Vec3::Zero;
        }
    }

    // generate indices
    int k1, k2;

    for (int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1); // beginning of current stack
        k2 = k1 + sectorCount + 1;  // beginning of next stack

        // the sectors in first and last stack only has 1 triangle
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // k1 => k2 => k1+1
            if (i != 0)
            {
                mesh.Indices.PushBack(k1);
                mesh.Indices.PushBack(k2);
                mesh.Indices.PushBack(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1))
            {
                mesh.Indices.PushBack(k1 + 1);
                mesh.Indices.PushBack(k2);
                mesh.Indices.PushBack(k2 + 1);
            }
        }
    }
}

} // namespace LD