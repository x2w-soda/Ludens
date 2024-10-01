#pragma once

#include "Core/Header/Include/Error.h"
#include "Core/PhysicsBase/Include/Jolt/JoltTypes.h"
#include "Core/PhysicsBase/Include/Jolt/JoltShape.h"

namespace LD
{

JPH::ShapeRefC JoltShape::GetRef()
{
    if (mShapeRef == nullptr)
    {
        mShapeRef = CreateShape();
        LD_DEBUG_ASSERT(mShapeRef);
    }

    return mShapeRef;
}

JoltSphereShape::JoltSphereShape(float radius) : mRadius(radius)
{
    LD_DEBUG_ASSERT(radius > 0.0f);
}

JPH::ShapeRefC JoltSphereShape::CreateShape()
{
    const JPH::SphereShapeSettings settings(mRadius);
    const JPH::ShapeSettings::ShapeResult result = settings.Create();
    
    LD_DEBUG_ASSERT(!result.HasError());

    return result.Get();
}

JoltBoxShape::JoltBoxShape(const Vec3& halfExtent) : mHalfExtent(halfExtent)
{
}

JPH::ShapeRefC JoltBoxShape::CreateShape()
{
    const JPH::BoxShapeSettings settings(ToJolt(mHalfExtent));
    const JPH::ShapeSettings::ShapeResult result = settings.Create();

    LD_DEBUG_ASSERT(!result.HasError());

    return result.Get();
}

} // namespace LD