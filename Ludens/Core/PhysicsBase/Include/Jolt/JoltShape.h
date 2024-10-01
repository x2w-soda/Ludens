#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include "Core/Math/Include/Vec3.h"

namespace LD
{

// Jolt Shapes are serializable and ref counted objects,
// this abstraction layer provides error handling and debug assertions
// over shape setting creation.

class JoltShape
{
public:
    JoltShape() = default;
    virtual ~JoltShape() = default;

    JPH::ShapeRefC GetRef();

protected:
    virtual JPH::ShapeRefC CreateShape() = 0;

    JPH::ShapeRefC mShapeRef = nullptr;
};

class JoltSphereShape final : public JoltShape
{
public:
    JoltSphereShape(float radius);

protected:
    virtual JPH::ShapeRefC CreateShape() override;

private:
    float mRadius;
};

class JoltBoxShape final : public JoltShape
{
public:
    JoltBoxShape(const Vec3& halfExtent = { 1.0f, 1.0f, 1.0f });

protected:
    virtual JPH::ShapeRefC CreateShape() override;

private:
    Vec3 mHalfExtent;
};

} // namespace LD