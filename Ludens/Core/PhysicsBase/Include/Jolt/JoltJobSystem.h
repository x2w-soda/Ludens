#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace LD
{

class JoltJobSystem : public JPH::JobSystemWithBarrier
{
public:
    JoltJobSystem();
    JoltJobSystem(const JoltJobSystem&) = delete;
    ~JoltJobSystem() = default;

    JoltJobSystem& operator=(const JoltJobSystem&) = delete;

    void Startup(unsigned int maxPhysicsJobs);
    void Cleanup();

private:
    virtual int GetMaxConcurrency() const override;

    virtual JPH::JobHandle CreateJob(const char* p_name, JPH::ColorArg color,
                                     const JPH::JobSystem::JobFunction& jobFunction,
                                     JPH::uint32 dependencyArg = 0) override;

    virtual void FreeJob(JPH::JobSystem::Job* job) override;

    virtual void QueueJob(JPH::JobSystem::Job* job) override;

    virtual void QueueJobs(JPH::JobSystem::Job** jobs, JPH::uint jobCount) override;

    class JoltJob : public JPH::JobSystem::Job
    {
    public:
        JoltJob(const char* name, JPH::ColorArg color, JPH::JobSystem* jobSystem,
                const JPH::JobSystem::JobFunction& jobFunction, JPH::uint32 dependencyCount);

        JoltJob(const JoltJob&) = delete;
        JoltJob(JoltJob&&) = delete;
        ~JoltJob();

        JoltJob& operator=(const JoltJob&) = delete;
        JoltJob& operator=(JoltJob&&) = delete;

        void Queue();

    private:
        static void ExecuteJolt(void*);
    };

    bool mHasStartup;
    JPH::FixedSizeFreeList<JoltJob> mFreeList;
};

} // namespace LD