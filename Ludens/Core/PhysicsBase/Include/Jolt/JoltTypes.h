#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include "Core/Math/Include/Vec3.h"
#include "Core/Math/Include/Quat.h"
#include "Core/PhysicsBase/Include/Jolt/JoltShape.h"
#include "Core/PhysicsBase/Include/Jolt/JoltPhysicsSystem.h"
#include "Core/PhysicsBase/Include/PTypes.h"

// Type Conversions
// - Jolt is by default compiled with 32-bit floating point precision
// - Jolt matrices are column major
// - Jolt quaternion real part stored at w component

namespace LD
{

inline JPH::Vec3 ToJolt(const Vec3& v)
{
    return JPH::Vec3(v.x, v.y, v.z);
}

inline Vec3 FromJolt(const JPH::Vec3& v)
{
    return LD::Vec3(v.GetX(), v.GetY(), v.GetZ());
}

inline JPH::Quat ToJolt(const Quat& q)
{
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

inline Quat FromJolt(const JPH::Quat& q)
{
    return LD::Quat(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
}

inline JPH::ShapeRefC ToJolt(const PShape& shape)
{
    switch (shape.Type)
    {
    case PShapeType::Box:
        return JoltBoxShape(static_cast<const PBoxShape&>(shape).HalfExtent).GetRef();
    case PShapeType::Sphere:
        return JoltSphereShape(static_cast<const PSphereShape&>(shape).Radius).GetRef();
    }

    return nullptr;
}

inline JPH::EMotionType ToJoltMotionType(PMotionType motionType)
{
    switch (motionType)
    {
    case PMotionType::Static:
        return JPH::EMotionType::Static;
    case PMotionType::Dynamic:
        return JPH::EMotionType::Dynamic;
    case PMotionType::Kinematic:
        return JPH::EMotionType::Kinematic;
    }

    LD_DEBUG_UNREACHABLE;
    return JPH::EMotionType::Static;
}

inline JPH::ObjectLayer ToJoltObjectLayer(PMotionType motionType)
{
    switch (motionType)
    {
    case PMotionType::Static:
        return JoltObjectLayer::Static;
    case PMotionType::Dynamic:
        return JoltObjectLayer::Dynamic;
    }

    LD_DEBUG_UNREACHABLE;
    return JoltObjectLayer::Static;
}

} // namespace LD