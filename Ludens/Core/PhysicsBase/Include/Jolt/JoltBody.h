#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include "Core/PhysicsBase/Include/Jolt/JoltShape.h"

namespace LD
{

class JoltPhysicsSystem;

class JoltBodyLock
{
public:
    JoltBodyLock() = delete;
    JoltBodyLock(const JoltPhysicsSystem* system);
    JoltBodyLock(const JoltBodyLock&) = delete;
    JoltBodyLock(JoltBodyLock&&) = delete;
    virtual ~JoltBodyLock();

    JoltBodyLock& operator=(const JoltBodyLock&) = delete;
    JoltBodyLock& operator=(JoltBodyLock&&) = delete;

    void Acquire(const JPH::BodyID& id);

    void Release();

protected:
    virtual void AcquireImpl(const JPH::BodyID& id) = 0;
    virtual void ReleaseImpl() = 0;

    JPH::BodyLockInterface::MutexMask mMutexMask = 0;
    const JPH::BodyLockInterface* mLockInterface = nullptr;
    const JoltPhysicsSystem* mSystem = nullptr;
};

class JoltBodyReader : public JoltBodyLock
{
public:
    JoltBodyReader() = delete;
    JoltBodyReader(const JoltPhysicsSystem& system);
    ~JoltBodyReader();

    const JPH::Body* TryGetBody(const JPH::BodyID& id) const;

private:
    virtual void AcquireImpl(const JPH::BodyID& id) override;
    virtual void ReleaseImpl() override;
};

class JoltBodyWriter : public JoltBodyLock
{
public:
    JoltBodyWriter() = delete;
    JoltBodyWriter(const JoltPhysicsSystem& system);
    ~JoltBodyWriter();

    JPH::Body* TryGetBody(const JPH::BodyID& id) const;

private:
    virtual void AcquireImpl(const JPH::BodyID& id) override;
    virtual void ReleaseImpl() override;
};

template <typename TLock>
class JoltBodyScopedLock
{
public:
    JoltBodyScopedLock() = delete;
    JoltBodyScopedLock(const JoltPhysicsSystem& system, const JPH::BodyID& id) : mLock(system)
    {
        mLock.Acquire(id);
    }

    ~JoltBodyScopedLock()
    {
        mLock.Release();
    }

    inline auto TryGetBody(const JPH::BodyID& id) const
    {
        return mLock.TryGetBody(id);
    }

private:
    TLock mLock;
};

template <typename TScopedLock, typename TBody>
class JoltBodyAccessResult
{
public:
    JoltBodyAccessResult() = delete;
    JoltBodyAccessResult(const JoltPhysicsSystem& system, const JPH::BodyID& id) : mScopedLock(system, id)
    {
        mResult = mScopedLock.TryGetBody(id);
    }

    bool Succeeded() const
    {
        return mResult != nullptr;
    }

    TBody* operator->() const
    {
        LD_DEBUG_ASSERT(mResult);
        return mResult;
    }

    TBody& operator*() const
    {
        LD_DEBUG_ASSERT(mResult);
        return mResult;
    }

private:
    TScopedLock mScopedLock;
    TBody* mResult;
};

using JoltBodyScopedReader = JoltBodyScopedLock<JoltBodyReader>;
using JoltBodyScopedWriter = JoltBodyScopedLock<JoltBodyWriter>;

using JoltBodyReadResult = JoltBodyAccessResult<JoltBodyScopedReader, const JPH::Body>;
using JoltBodyWriteResult = JoltBodyAccessResult<JoltBodyScopedWriter, JPH::Body>;

} // namespace LD