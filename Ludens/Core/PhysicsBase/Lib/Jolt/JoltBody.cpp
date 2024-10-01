#include "Core/PhysicsBase/Include/Jolt/JoltBody.h"
#include "Core/PhysicsBase/Include/Jolt/JoltPhysicsSystem.h"

namespace LD
{

JoltBodyLock::JoltBodyLock(const JoltPhysicsSystem* system) : mSystem(system), mLockInterface(nullptr)
{
}

JoltBodyLock::~JoltBodyLock() = default;

void JoltBodyLock::Acquire(const JPH::BodyID& id)
{
    LD_DEBUG_ASSERT(mSystem && !mLockInterface);

    mLockInterface = &mSystem->GetBodyLockInterface();
    AcquireImpl(id);
}

void JoltBodyLock::Release()
{
    LD_DEBUG_ASSERT(mSystem && mLockInterface);

    ReleaseImpl();
    mLockInterface = nullptr;
}

JoltBodyReader::JoltBodyReader(const JoltPhysicsSystem& system) : JoltBodyLock(&system)
{
}

JoltBodyReader::~JoltBodyReader()
{
}

const JPH::Body* JoltBodyReader::TryGetBody(const JPH::BodyID& id) const
{
    return mLockInterface->TryGetBody(id);
}

void JoltBodyReader::AcquireImpl(const JPH::BodyID& id)
{
    mMutexMask = mLockInterface->GetMutexMask(&id, 1);
    mLockInterface->LockRead(mMutexMask);
}

void JoltBodyReader::ReleaseImpl()
{
    mLockInterface->UnlockRead(mMutexMask);
    mMutexMask = 0;
}

JoltBodyWriter::JoltBodyWriter(const JoltPhysicsSystem& system) : JoltBodyLock(&system)
{
}

JoltBodyWriter::~JoltBodyWriter()
{
}

JPH::Body* JoltBodyWriter::TryGetBody(const JPH::BodyID& id) const
{
    return mLockInterface->TryGetBody(id);
}

void JoltBodyWriter::AcquireImpl(const JPH::BodyID& id)
{
    mMutexMask = mLockInterface->GetMutexMask(&id, 1);
    mLockInterface->LockWrite(mMutexMask);
}

void JoltBodyWriter::ReleaseImpl()
{
    mLockInterface->UnlockWrite(mMutexMask);
    mMutexMask = 0;
}

} // namespace LD