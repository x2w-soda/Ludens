#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/System/Memory.h>

using namespace LD;

class IncJob
{
public:
    IncJob()
        : mValue(0)
    {
        mHeader.fn = &IncJob::main;
        mHeader.user = this;
    }

    void set_value(int value)
    {
        mValue = value;
    }

    int get_value() const
    {
        return mValue;
    }

private:
    static void main(void* user)
    {
        IncJob* self = (IncJob*)user;
        self->mValue++; // it aint much, but it's honest work
    }

private:
    JobHeader mHeader;
    int mValue;
};

/// @brief fibonacci job, worker threads can spawn jobs but never wait on jobs.
class FibJob
{
public:
    FibJob()
        : mQuery(0), mResult(-1), mChild0(nullptr), mChild1(nullptr)
    {
        mHeader.fn = &FibJob::main;
        mHeader.user = this;
    }

    ~FibJob()
    {
        if (mChild0)
            heap_delete<FibJob>(mChild0);

        if (mChild1)
            heap_delete<FibJob>(mChild1);
    }

    /// @brief submit to job system
    /// @param fibQuery index of fibonacci sequence to evaluate
    void submit(int fibQuery)
    {
        mQuery = fibQuery;
        JobSystem::get().submit(&mHeader, JOB_DISPATCH_STANDARD);
    }

    /// @brief only call this after main thread has waited for FibJobs to complete
    /// @return the fibonacci element requested.
    int get_result()
    {
        if (mChild0 && mChild1)
        {
            // main thread must wait until all FibJobs complete.
            mResult = mChild0->get_result() + mChild1->get_result();
            heap_delete<FibJob>(mChild0);
            heap_delete<FibJob>(mChild1);
            mChild0 = nullptr;
            mChild1 = nullptr;
        }

        return mResult;
    }

private:
    static void main(void* user)
    {
        FibJob* job = (FibJob*)user;

        // let F0 = 0, F1 = 1
        if (job->mQuery <= 1)
        {
            job->mResult = job->mQuery;
            return;
        }

        // worker thread can spawn other jobs, but not wait for their results.
        job->mChild0 = heap_new<FibJob>(MEMORY_USAGE_JOB_SYSTEM);
        job->mChild0->submit(job->mQuery - 2);
        job->mChild1 = heap_new<FibJob>(MEMORY_USAGE_JOB_SYSTEM);
        job->mChild1->submit(job->mQuery - 1);
    }

private:
    JobHeader mHeader;
    FibJob* mChild0 = nullptr;
    FibJob* mChild1 = nullptr;
    int mQuery;
    int mResult;
};

TEST_CASE("JobSystem")
{
    JobSystem::init({
        .immediateQueueCapacity = 10,
        .standardQueueCapacity = 10,
    });

    JobSystem js = JobSystem::get();

    IncJob job1;
    job1.set_value(3);
    js.submit((JobHeader*)&job1, JOB_DISPATCH_IMMEDIATE);
    js.wait_all();

    CHECK(job1.get_value() == 4);

    IncJob job2;
    job2.set_value(4);
    js.submit((JobHeader*)&job2, JOB_DISPATCH_STANDARD);
    js.wait_all();

    CHECK(job2.get_value() == 5);

    JobSystem::shutdown();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_JOB_SYSTEM);
    CHECK(profile.current == 0);
}

template <size_t TCapacity, size_t N>
void test_bandwidth()
{
    JobSystem::init({
        .immediateQueueCapacity = TCapacity,
        .standardQueueCapacity = TCapacity,
    });

    JobSystem js = JobSystem::get();

    std::vector<IncJob> immJobs(N);
    std::vector<IncJob> stdJobs(N);

    for (size_t i = 0; i < N; i++)
    {
        immJobs[i].set_value((int)i);
        stdJobs[i].set_value((int)i);

        js.submit((JobHeader*)(immJobs.data() + i), JOB_DISPATCH_IMMEDIATE);
        js.submit((JobHeader*)(stdJobs.data() + i), JOB_DISPATCH_STANDARD);
    }

    js.wait_all();

    for (size_t i = 0; i < N; i++)
    {
        CHECK(immJobs[i].get_value() == (int)i + 1);
        CHECK(stdJobs[i].get_value() == (int)i + 1);
    }

    JobSystem::shutdown();
}

TEST_CASE("JobSystem bandwidth")
{
    test_bandwidth<2, 256>();
    test_bandwidth<256, 256>();
    test_bandwidth<512, 256>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_JOB_SYSTEM);
    CHECK(profile.current == 0);
}

template <size_t TCapacity>
void test_worker_spawn_jobs()
{
    JobSystem::init({
        .immediateQueueCapacity = TCapacity,
        .standardQueueCapacity = TCapacity,
    });

    FibJob job1;
    job1.submit(10);

    FibJob job2;
    job2.submit(19);

    FibJob job3;
    job3.submit(25);

    JobSystem js = JobSystem::get();
    js.wait_all();

    int result = job1.get_result();
    CHECK(result == 55);

    result = job2.get_result();
    CHECK(result == 4181);

    result = job3.get_result();
    CHECK(result == 75025);

    JobSystem::shutdown();
}

TEST_CASE("JobSystem worker spawns jobs")
{
    test_worker_spawn_jobs<2>();
    test_worker_spawn_jobs<512>();
    test_worker_spawn_jobs<8192>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_JOB_SYSTEM);
    CHECK(profile.current == 0);
}