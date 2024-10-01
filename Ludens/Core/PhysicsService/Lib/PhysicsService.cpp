#include <unordered_map>
#include "Core/OS/Include/Time.h"
#include "Core/OS/Include/JobSystem.h"
#include "Core/PhysicsBase/Include/Jolt/JoltPhysics.h"
#include "Core/PhysicsBase/Include/Jolt/JoltTypes.h"
#include "Core/PhysicsBase/Include/Jolt/JoltPhysicsSystem.h"
#include "Core/PhysicsBase/Include/Jolt/JoltJobSystem.h"
#include "Core/PhysicsService/Include/PhysicsService.h"

namespace LD
{

static JoltPhysicsSystem* sSystem;
static std::unordered_map<PRID, JPH::BodyID> sIDTable;

void PhysicsService::Startup()
{
    JoltPhysics::Startup();

    sSystem = new JoltPhysicsSystem();
    sSystem->Startup();
}

void PhysicsService::Cleanup()
{
    sSystem->Cleanup();
    delete sSystem;

    JoltPhysics::Cleanup();
}

void PhysicsService::Update(DeltaTime& dt)
{
    sSystem->Update(dt);

    JobSystem::GetSingleton().WaitType(JobType::Physics);
}

void PhysicsService::CreateRigidBody(PRID& id, const Vec3& position, const Quat& rotation, const PShape& shape,
                                     PMotionType motionType)
{
    id = CUID<JPH::BodyID>::Get();

    JPH::ShapeRefC shapeRef = ToJolt(shape);
    LD_DEBUG_ASSERT(shapeRef != nullptr);

    JPH::BodyCreationSettings settings(shapeRef, ToJolt(position), ToJolt(rotation), ToJoltMotionType(motionType),
                                       ToJoltObjectLayer(motionType));

    JPH::BodyID bodyID = sSystem->AddRigidBody(settings);
    sIDTable[id] = bodyID;
}

void PhysicsService::DeleteRigidBody(PRID id)
{
    LD_DEBUG_ASSERT(sIDTable.find(id) != sIDTable.end());

    sSystem->RemoveBody(sIDTable[id]);
    sIDTable.erase(id);
}

bool PhysicsService::GetBodyPosition(PRID id, Vec3& position)
{
    LD_DEBUG_ASSERT(sIDTable.find(id) != sIDTable.end());

    JoltBodyReadResult result = sSystem->ReadBody(sIDTable[id]);

    if (!result.Succeeded())
        return false;

    position = FromJolt(result->GetPosition());
    return true;
}

bool PhysicsService::GetBodyRotation(PRID id, Quat& rotation)
{
    LD_DEBUG_ASSERT(sIDTable.find(id) != sIDTable.end());

    JoltBodyReadResult result = sSystem->ReadBody(sIDTable[id]);

    if (!result.Succeeded())
        return false;

    rotation = FromJolt(result->GetRotation());
    return true;
}

bool PhysicsService::GetBodyTransform(PRID id, Vec3& position, Quat& rotation)
{
    LD_DEBUG_ASSERT(sIDTable.find(id) != sIDTable.end());
    JoltBodyReadResult result = sSystem->ReadBody(sIDTable[id]);

    if (!result.Succeeded())
        return false;

    position = FromJolt(result->GetPosition());
    rotation = FromJolt(result->GetRotation());
    return true;
}

bool PhysicsService::SetBodyLinearVelocity(PRID id, const Vec3& velocity)
{
    LD_DEBUG_ASSERT(sIDTable.find(id) != sIDTable.end());

    JoltBodyWriteResult result = sSystem->WriteBody(sIDTable[id]);

    if (!result.Succeeded())
        return false;

    result->SetLinearVelocity(ToJolt(velocity));
    return true;
}

} // namespace LD