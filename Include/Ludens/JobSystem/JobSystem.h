#pragma once

#include <Ludens/Header/Handle.h>
#include <cstdint>
#include <cstdlib>

namespace LD {

enum JobDispatchType
{
    JOB_DISPATCH_IMMEDIATE = 0,
    JOB_DISPATCH_STANDARD,
};

typedef void (*JobFn)(void* user);

struct JobHeader
{
    uint32_t type;
    JobFn onExecute;  // job body, runs on worker thread
    JobFn onComplete; // optional hook, runs on worker thread after job body
    void* user;       // dependency injection during callbacks
};

struct JobSystemInfo
{
    size_t immediateQueueCapacity;
    size_t standardQueueCapacity;
};

/// @brief Thread-based Job System, using one main thread and multiple worker threads.
///        All threads may create new jobs, but only the main thread can wait for jobs to finish.
struct JobSystem : Handle<struct JobSystemObj>
{
    static void init(const JobSystemInfo& info);
    static void shutdown();

    /// @brief get singleton handle
    /// @return job system handle
    static JobSystem get();

    /// @brief get number of worker threads
    int get_worker_thread_count();

    /// @brief wait for worker threads to complete all jobs
    /// @warning can only be called from main thread,
    ///          worker threads waiting for jobs may
    ///          lead to deadlocks.
    void wait_all();

    void submit(const JobHeader* job, JobDispatchType type);

    /// @brief Move jobs of the specified type to the very front of each job queue.
    ///        Note that the immediate queue still takes priority over other queues.
    /// @param type the job type to prioritize
    void prioritize(uint32_t type);
};

} // namespace LD
