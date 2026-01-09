#include <Ludens/DSA/Vector.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace LD {

/// thread safe ring buffer to store JobHeader info
class JobQueue
{
public:
    JobQueue(size_t capacity)
        : mCap(capacity), mHead(0), mTail(0)
    {
        mJobs = (JobHeader*)heap_malloc(sizeof(JobHeader) * capacity, MEMORY_USAGE_JOB_SYSTEM);
    }

    ~JobQueue()
    {
        heap_free(mJobs);
    }

    inline bool enqueue(const JobHeader& job)
    {
        bool success = false;

        {
            std::unique_lock<std::mutex> lock(mMutex);

            size_t next = (mHead + 1) % mCap;

            if (next != mTail)
            {
                mJobs[mHead] = job;
                mHead = next;
                mSize.fetch_add(1);
                success = true;
            }
        }

        return success;
    }

    inline bool dequeue(JobHeader& job)
    {
        bool success = false;

        {
            std::unique_lock<std::mutex> lock(mMutex);

            if (mHead != mTail)
            {
                job = mJobs[mTail];
                mTail = (mTail + 1) % mCap;
                mSize.fetch_sub(1);
                success = true;
            }
        }

        return success;
    }

    inline size_t size()
    {
        return mSize;
    }

    inline bool empty()
    {
        return mSize == 0;
    }

    void prioritize(uint32_t prioType)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        size_t front = mTail;

        for (size_t now = mTail; now != mHead; now = (now + 1) % mCap)
        {
            if (mJobs[now].type == prioType)
            {
                JobHeader tmp = mJobs[front];
                mJobs[front] = mJobs[now];
                mJobs[now] = tmp;

                front = (front + 1) % mCap;
            }
        }
    }

private:
    std::mutex mMutex;
    std::atomic<size_t> mSize;
    const size_t mCap;
    size_t mHead;
    size_t mTail;
    JobHeader* mJobs;
};

struct WorkerThread
{
    std::thread handle;
    std::atomic_bool isWorking;
};

/// @brief thread-based job system implementation
struct JobSystemObj
{
    std::condition_variable wakeCV;    // CV to wake worker threads
    std::condition_variable waitAllCV; // CV to wait until all jobs complete
    std::mutex wakeMutex;              // used in conjunction with wakeCV
    std::mutex waitAllMutex;           // used in conjunction with waitAllCV
    std::vector<WorkerThread*> workerThreads;
    std::atomic<bool> isRunning;
    std::atomic<size_t> jobCounter;
    JobQueue immQueue;
    JobQueue stdQueue;

    JobSystemObj(const JobSystemInfo& info);
    ~JobSystemObj();
};

static JobSystemObj* sObj;

static void worker_thread_main(void* thread);

static void execute_job(const JobHeader& job)
{
    job.fn(job.user);

    if (sObj->jobCounter.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        std::unique_lock<std::mutex> lock(sObj->waitAllMutex);
        sObj->waitAllCV.notify_one();
    }
}

JobSystemObj::JobSystemObj(const JobSystemInfo& info)
    : immQueue(info.immediateQueueCapacity), stdQueue(info.standardQueueCapacity)
{
    int workerCount = (int)std::thread::hardware_concurrency() - 1;
    workerThreads.resize(workerCount);

    for (int i = 0; i < workerCount; i++)
    {
        workerThreads[i] = heap_new<WorkerThread>(MEMORY_USAGE_JOB_SYSTEM);
        workerThreads[i]->isWorking = false;
    }

    isRunning = true;
}

JobSystemObj::~JobSystemObj()
{
    isRunning = false;

    wakeCV.notify_all();

    for (WorkerThread* thread : workerThreads)
    {
        thread->handle.join();
        heap_delete<WorkerThread>(thread);
    }
}

static void worker_thread_main(void* user)
{
    WorkerThread* thread = (WorkerThread*)user;

    while (sObj->isRunning)
    {
        JobHeader job;
        bool foundJob = sObj->immQueue.dequeue(job) || sObj->stdQueue.dequeue(job);

        if (foundJob)
        {
            thread->isWorking = true;
            execute_job(job);
            thread->isWorking = false;
        }
        else
        {
            // put worker thread to sleep
            std::unique_lock<std::mutex> lock(sObj->wakeMutex);
            sObj->wakeCV.wait(lock, [] {
                bool hasJob = !sObj->immQueue.empty() || !sObj->stdQueue.empty();
                return hasJob || !sObj->isRunning;
            });
        }
    }
}

void JobSystem::init(const JobSystemInfo& info)
{
    if (sObj)
        return;

    sObj = heap_new<JobSystemObj>(MEMORY_USAGE_JOB_SYSTEM, info);

    for (WorkerThread* thread : sObj->workerThreads)
        thread->handle = std::thread(&worker_thread_main, thread);
}

void JobSystem::shutdown()
{
    if (!sObj)
        return;

    heap_delete<JobSystemObj>(sObj);
    sObj = nullptr;
}

JobSystem JobSystem::get()
{
    return {sObj};
}

int JobSystem::get_worker_thread_count()
{
    return (int)mObj->workerThreads.size();
}

void JobSystem::wait_all()
{
    LD_PROFILE_SCOPE;

    bool allDispatched;

    do
    {
        mObj->wakeCV.notify_one();

        allDispatched = mObj->immQueue.empty() && mObj->stdQueue.empty();
    } while (!allDispatched);

    std::unique_lock<std::mutex> lock(mObj->waitAllMutex);
    mObj->waitAllCV.wait(lock, [] { return sObj->jobCounter.load(std::memory_order_acquire) == 0; });
}

void JobSystem::submit(const JobHeader* job, JobDispatchType type)
{
    JobQueue* queue = (type == JOB_DISPATCH_IMMEDIATE) ? &mObj->immQueue : &mObj->stdQueue;

    if (queue->enqueue(*job))
    {
        sObj->jobCounter.fetch_add(1);
        sObj->wakeCV.notify_one();
    }
    else
    {
        sObj->jobCounter.fetch_add(1);

        // TODO: currently it is possible that the main thread
        //       starts executing a large job and freezes the app.
        execute_job(*job);
    }
}

void JobSystem::prioritize(uint32_t type)
{
    mObj->immQueue.prioritize(type);
    mObj->stdQueue.prioritize(type);
}

} // namespace LD