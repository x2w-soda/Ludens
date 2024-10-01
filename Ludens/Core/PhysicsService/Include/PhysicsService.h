#pragma once

#include "Core/Header/Include/Singleton.h"
#include "Core/OS/Include/UID.h"
#include "Core/Math/Include/Quat.h"
#include "Core/PhysicsBase/Include/PTypes.h"

namespace LD
{

/// physics engine resource id
using PRID = UID;

class DeltaTime;

class PhysicsService : public Singleton<PhysicsService>
{
    friend class Singleton<PhysicsService>;

public:
    void Startup();
    void Cleanup();

    void Update(DeltaTime& dt);

    void CreateRigidBody(PRID& id, const Vec3& position, const Quat& rotation, const PShape& shape, PMotionType motionType);

    void DeleteRigidBody(PRID id);

    bool GetBodyPosition(PRID id, Vec3& position);

    bool GetBodyRotation(PRID id, Quat& rotation);

    bool GetBodyTransform(PRID id, Vec3& position, Quat& rotation);
    
    bool SetBodyLinearVelocity(PRID id, const Vec3& velocity);

private:
    PhysicsService() = default;
};

} // namespace LD