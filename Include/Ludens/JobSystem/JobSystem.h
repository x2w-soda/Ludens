#pragma once

#include <Ludens/Header/Handle.h>
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
    JobFn fn;
    void* user;
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
};

} // namespace LD
