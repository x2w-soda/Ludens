#pragma once

#include "Core/OS/Include/JobSystem.h"
#include "Core/PhysicsBase/Include/Jolt/JoltJobSystem.h"

namespace LD
{

JoltJobSystem::JoltJobSystem() : JPH::JobSystemWithBarrier(JPH::cMaxPhysicsBarriers)
{
}

void JoltJobSystem::Startup(unsigned int maxPhysicsJobs)
{
    mFreeList.Init(maxPhysicsJobs, maxPhysicsJobs);
    mHasStartup = true;
}

void JoltJobSystem::Cleanup()
{
    mHasStartup = false;
}

int JoltJobSystem::GetMaxConcurrency() const
{
    auto& js = LD::JobSystem::GetSingleton();

    return js.GetWorkerThreadCount();
}

JPH::JobHandle JoltJobSystem::CreateJob(const char* name, JPH::ColorArg color,
                                        const JPH::JobSystem::JobFunction& jobFunction, JPH::uint32 dependencyCount)
{
    LD_DEBUG_ASSERT(mHasStartup);
    LD_DEBUG_ASSERT(jobFunction != nullptr);

    JoltJob* job = nullptr;

    while (true)
    {
        const JPH::uint32 objIndex = mFreeList.ConstructObject(name, color, this, jobFunction, dependencyCount);

        if (objIndex == JPH::FixedSizeFreeList<JoltJob>::cInvalidObjectIndex)
            job = nullptr;
        else
        {
            job = &mFreeList.Get(objIndex);
            break;
        }

        LD_DEBUG_UNREACHABLE;
    }

    JPH::JobHandle handle(job);

    if (dependencyCount == 0)
        QueueJob(job);

    return handle;
}

void JoltJobSystem::FreeJob(JPH::JobSystem::Job* job)
{
    mFreeList.DestructObject((JoltJob*)job);
}

void JoltJobSystem::QueueJob(JPH::JobSystem::Job* job)
{
    LD_DEBUG_ASSERT(job);

    static_cast<JoltJob*>(job)->Queue();
}

void JoltJobSystem::QueueJobs(JPH::JobSystem::Job** jobs, JPH::uint jobCount)
{
    for (JPH::uint i = 0; i < jobCount; ++i)
    {
        QueueJob(jobs[i]);
    }
}

JoltJobSystem::JoltJob::JoltJob(const char* name, JPH::ColorArg color, JPH::JobSystem* jobSystem,
                                const JPH::JobSystem::JobFunction& jobFunction, JPH::uint32 dependencyCount)
    : JPH::JobSystem::Job(name, color, jobSystem, jobFunction, dependencyCount)
{
    LD_DEBUG_ASSERT(jobFunction != nullptr);
}

JoltJobSystem::JoltJob::~JoltJob()
{
}

void JoltJobSystem::JoltJob::Queue()
{
    LD::JobSystem& js = LD::JobSystem::GetSingleton();

    LD::Job job;
    job.Type = LD::JobType::Physics;
    job.Data = this;
    job.Main = &JoltJobSystem::JoltJob::ExecuteJolt;

    js.Submit(job);
}

void JoltJobSystem::JoltJob::ExecuteJolt(void* userdata)
{
    JoltJob* job = static_cast<JoltJob*>(userdata);

    job->Execute();
    job->Release();
}

} // namespace LD