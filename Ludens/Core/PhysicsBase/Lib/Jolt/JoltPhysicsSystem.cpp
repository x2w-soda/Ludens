#include "Core/PhysicsBase/Include/Jolt/JoltPhysicsSystem.h"

namespace LD
{

JoltPhysicsSystem::JoltPhysicsSystem() : mTempAllocator(10 * 1024 * 1024)
{
}

JoltPhysicsSystem::~JoltPhysicsSystem()
{
}

void JoltPhysicsSystem::Startup()
{
    mJobSystem.Startup(JPH::cMaxPhysicsJobs);

    const unsigned int cMaxBodies = 1024;
    const unsigned int cMaxBodyPairs = 1024;
    const unsigned int cMaxContactConstraints = 1024;

    // sTestJobSystem = new JPH::JobSystemThreadPool(cMaxBodies, JPH::cMaxPhysicsBarriers,
    //                                                  std::thread::hardware_concurrency() - 1);

    mPhysics.Init(cMaxBodies, 0, cMaxBodyPairs, cMaxContactConstraints, mBroadPhaseLayerInterface,
                  mObjectVsBroadPhaseLayerFilter, mOjectLayerPairFilter);
}

void JoltPhysicsSystem::Cleanup()
{
    mJobSystem.Cleanup();
}

void JoltPhysicsSystem::Update(DeltaTime dt)
{
    const JPH::EPhysicsUpdateError error = mPhysics.Update(dt.GetSeconds(), 1, &mTempAllocator, &mJobSystem);
}

JPH::BodyInterface& JoltPhysicsSystem::GetBodyInterface()
{
    return mPhysics.GetBodyInterfaceNoLock();
}

const JPH::BodyLockInterface& JoltPhysicsSystem::GetBodyLockInterface() const
{
    return mPhysics.GetBodyLockInterface();
}

JPH::BodyID JoltPhysicsSystem::AddRigidBody(const JPH::BodyCreationSettings& settings)
{
    const JPH::BodyID bodyID = GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);

    LD_DEBUG_ASSERT(!bodyID.IsInvalid());

    return bodyID;
}

void JoltPhysicsSystem::RemoveBody(const JPH::BodyID& id)
{
    LD_DEBUG_ASSERT(!id.IsInvalid());

    JPH::BodyInterface& interface = GetBodyInterface();
    interface.RemoveBody(id);
    interface.DestroyBody(id);
}

JoltBodyReadResult JoltPhysicsSystem::ReadBody(const JPH::BodyID& id) const
{
    return { *this, id };
}

JoltBodyWriteResult JoltPhysicsSystem::WriteBody(const JPH::BodyID& id) const
{
    return { *this, id };
}

} // namespace LD