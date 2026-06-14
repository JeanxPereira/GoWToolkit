#include <doctest/doctest.h>

#include <atomic>
#include <thread>

#include <Onyx/Services/Threading.h>

namespace T = Onyx::Threading;

TEST_CASE("[Threading] MarkMainThread + IsMainThread agree on the calling thread") {
    T::MarkMainThread();
    CHECK(T::IsMainThread());
}

TEST_CASE("[Threading] Worker thread sees IsMainThread() == false") {
    T::MarkMainThread();
    CHECK(T::IsMainThread());

    std::atomic<bool> workerResult{true}; // expect false after the worker runs
    std::thread worker([&] {
        workerResult.store(T::IsMainThread(), std::memory_order_relaxed);
    });
    worker.join();
    CHECK_FALSE(workerResult.load(std::memory_order_relaxed));

    // Main thread identity is preserved across worker lifetimes.
    CHECK(T::IsMainThread());
}

TEST_CASE("[Threading] ASSERT_MAIN_THREAD is a no-op in NDEBUG builds") {
#ifdef NDEBUG
    // Release: macro must expand to (void)0. We can verify by checking
    // that the expansion does not reference Threading at all â€” calling
    // it from a worker thread without first MarkMainThread()ing must
    // not abort.
    bool ranWithoutAbort = false;
    std::thread worker([&] {
        ASSERT_MAIN_THREAD();
        ranWithoutAbort = true;
    });
    worker.join();
    CHECK(ranWithoutAbort);
#else
    // Debug: the macro IS armed. We do not actually trigger it (the
    // assertion would terminate the process). Instead we verify the
    // helper it delegates to behaves as expected on this thread.
    T::MarkMainThread();
    ASSERT_MAIN_THREAD(); // must not fire on the main thread
    CHECK(T::IsMainThread());
#endif
}
