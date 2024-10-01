#include "Core/PhysicsBase/Include/PTypes.h"

namespace LD
{

PBoxShape::PBoxShape() : PShape(PShapeType::Box)
{
}

PBoxShape::PBoxShape(const Vec3& halfExtent) : PShape(PShapeType::Box), HalfExtent(halfExtent)
{
}

bool PBoxShape::IsValid() const
{
    return HalfExtent.x > 0.0f && HalfExtent.y > 0.0f && HalfExtent.z > 0.0f;
}

PSphereShape::PSphereShape() : PShape(PShapeType::Sphere), Radius(1.0f)
{
}

PSphereShape::PSphereShape(float radius) : PShape(PShapeType::Sphere), Radius(radius)
{
}

bool PSphereShape::IsValid() const
{
    return Radius > 0.0f;
}

} // namespace LD