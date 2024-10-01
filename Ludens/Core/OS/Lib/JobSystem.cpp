#include <thread>
#include <iostream>
#include "Core/DSA/Include/Optional.h"
#include "Core/OS/Include/JobSystem.h"
#include "Core/OS/Include/Mutex.h"

#define JOB_QUEUE_CAPACITY 2048

namespace LD
{

/// thread safe ring buffer to store Job info
class JobQueue
{
public:
    inline bool Enqueue(const Job& job)
    {
        bool success = false;
        mMutex.Lock();

        LD_DEBUG_ASSERT(job.Main);

        size_t next = (mHead + 1) % JOB_QUEUE_CAPACITY;
        if (next != mTail)
        {
            mArray[mHead] = job;
            mHead = next;
            mSize.fetch_add(1);
            success = true;
        }

        mMutex.Unlock();
        return success;
    }

    inline bool Dequeue(Job& job)
    {
        bool success = false;
        mMutex.Lock();

        if (mHead != mTail)
        {
            job = mArray[mTail];
            mTail = (mTail + 1) % JOB_QUEUE_CAPACITY;
            mSize.fetch_sub(1);
            success = true;
        }

        mMutex.Unlock();
        return success;
    }

    inline size_t GetSize()
    {
        return mSize;
    }

private:
    std::atomic<size_t> mSize;
    size_t mHead;
    size_t mTail;
    Mutex mMutex;
    Job mArray[JOB_QUEUE_CAPACITY];
};

static ConditionVariable sJobPending;
static JobQueue sJobQueues[(int)JobType::NUM_TYPES];
static std::atomic<size_t> sJobQueueIdx(0);
static std::atomic<size_t> sJobQueuePriorityIdx(0);
static std::atomic<bool> sUsePriorityIdx(false);
static std::atomic<bool> sIsAlive(false);

/// Job Scheduling Policy:
/// - Round Robin for all types of jobs
/// - During WaitType(), only allows dequeuing from the prioritized job queue
static bool AcquireJob(Job& job)
{
    size_t numTypes = (size_t)JobType::NUM_TYPES;

    if (sUsePriorityIdx)
        return sJobQueues[sJobQueuePriorityIdx].Dequeue(job);

    size_t idx = sJobQueueIdx.fetch_add(1) % numTypes;

    for (int i = 0; i < numTypes; i++)
    {
        if (sJobQueues[(idx + i) % numTypes].Dequeue(job))
            return true;
    }

    return false;
}


int JobSystem::JobThreadEntry(int id, void* userdata)
{
    JobThread* thread = (JobThread*)userdata;

    printf("Job Thread %d Online\n", id);

    while (true)
    {
        thread->Mutex.Lock();

        while (sIsAlive && !AcquireJob(thread->CurrentJob))
        {
            sJobPending.Wait(thread->Mutex);
        }

        if (!sIsAlive)
        {
            thread->Mutex.Unlock();
            break;
        }

        thread->IsWorking = true;
        thread->Mutex.Unlock();

        // execute the job
        LD_DEBUG_ASSERT(thread->CurrentJob.Main);
        thread->CurrentJob.Main(thread->CurrentJob.Data);
        
        thread->Mutex.Lock();
        thread->IsWorking = false;
        thread->Mutex.Unlock();
    }

    return 0;
}

JobSystem::JobSystem()
{
    mWorkerThreadCount = std::thread::hardware_concurrency() - 1;

    if (mWorkerThreadCount == 0)
        return;

    sIsAlive = true;
    mThreads.Resize(mWorkerThreadCount);

    for (JobThread& thread : mThreads)
    {
        thread.Run(&JobThreadEntry, (void*)&thread);
    }

    std::this_thread::sleep_for(std::chrono::microseconds(100));
}

JobSystem::~JobSystem()
{
    sIsAlive = false;
    sJobPending.SignalAll();

    for (JobThread& thread : mThreads)
    {
        thread.Stop();
    }

    mThreads.Clear();
}

int JobSystem::GetWorkerThreadCount()
{
    return mWorkerThreadCount;
}

void JobSystem::Submit(const Job& job)
{
    LD_DEBUG_ASSERT(job.Main);

    if (mWorkerThreadCount == 0)
    {
        job.Main(job.Data);
        return;
    }

    auto& queue = sJobQueues[(size_t)job.Type];

    while (!queue.Enqueue(job))
    {
        sJobPending.SignalOne();
    }
}

void JobSystem::WaitType(JobType type)
{
    if (mWorkerThreadCount == 0)
        return;

    sUsePriorityIdx = true;
    sJobQueuePriorityIdx = (size_t)type;

    while (sJobQueues[(size_t)type].GetSize() > 0)
    {
        sJobPending.SignalOne();
    }

    sUsePriorityIdx = false;

    while (true)
    {
        bool threadsIdle = true;

        for (JobThread& thread : mThreads)
        {
            thread.Mutex.Lock();

            if (thread.IsWorking && thread.CurrentJob.Type == type)
                threadsIdle = false;

            thread.Mutex.Unlock();
        }

        if (threadsIdle)
            break;
    }
}

void JobSystem::WaitAll()
{
    if (mWorkerThreadCount == 0)
        return;

    while (true)
    {
        bool allQueuesEmpty = true;

        for (size_t i = 0; i < (size_t)JobType::NUM_TYPES; i++)
        {
            if (sJobQueues[i].GetSize() > 0)
            {
                allQueuesEmpty = false;
                sJobPending.SignalOne();
                break;
            }
        }

        if (!allQueuesEmpty)
            continue;

        bool allThreadsIdle = true;

        for (JobThread& thread : mThreads)
        {
            thread.Mutex.Lock();

            if (thread.IsWorking)
                allThreadsIdle = false;
            
            thread.Mutex.Unlock();
        }

        if (allThreadsIdle)
            break;
    }
}

} // namespace LD