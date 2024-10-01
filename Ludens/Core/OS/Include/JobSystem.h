#pragma once

#include "Core/Header/Include/Singleton.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/OS/Include/UID.h"
#include "Core/OS/Include/Thread.h"
#include "Core/OS/Include/Mutex.h"
#include "Core/OS/Include/ConditionVariable.h"

namespace LD
{

enum class JobType
{
    Misc = 0,
    Physics = 1,
    LoadModel = 2,
    NUM_TYPES = 3,
};

struct Job
{
    JobType Type = JobType::Misc;

    /// the actual work to be done
    void (*Main)(void*);

    /// payload data
    void* Data;
};

class JobSystem : public Singleton<JobSystem>
{
    friend class Singleton<JobSystem>;

public:
    JobSystem(const JobSystem&) = delete;
    ~JobSystem();

    JobSystem& operator=(const JobSystem&) = delete;

    int GetWorkerThreadCount();

    void Submit(const Job& job);

    /// from the main thread, block until all jobs of a specific type is completed
    void WaitType(JobType type);

    /// from the main thread, block until all jobs are completed
    void WaitAll();

private:
    /// worker thread consuming jobs
    struct JobThread : public Thread
    {
        /// guard access to this object
        Mutex Mutex;

        Job CurrentJob;

        /// job is under execution
        bool IsWorking;
    };

    JobSystem();

    static int JobThreadEntry(int id, void* userdata);

    Vector<JobThread> mThreads;
    int mWorkerThreadCount;
};

} // namespace LD