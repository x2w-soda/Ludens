#pragma once

#include "Core/Math/Include/Vec3.h"

namespace LD
{

/// physics body motion types
enum class PMotionType
{
    Static,
    Dynamic,
    Kinematic,
};

/// physics 3D shape type
enum class PShapeType
{
    Box,
    Sphere,
};

struct PShape
{
    PShape() = delete;
    PShape(PShapeType type) : Type(type)
    {
    }
    virtual ~PShape() = default;

    const PShapeType Type;

    PShapeType GetType() const
    {
        return Type;
    }

    virtual bool IsValid() const = 0;
};

struct PBoxShape : PShape
{
    PBoxShape();
    PBoxShape(const Vec3& halfExtent);

    virtual bool IsValid() const override;

    Vec3 HalfExtent;
};

struct PSphereShape : PShape
{
    PSphereShape();
    PSphereShape(float radius);

    virtual bool IsValid() const override;

    float Radius;
};

} // namespace LD