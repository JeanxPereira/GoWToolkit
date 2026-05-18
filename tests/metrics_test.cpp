#include <doctest/doctest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "core/Metrics.h"

using namespace std::chrono_literals;
namespace M = GOW::Metrics;

TEST_CASE("[Metrics] Default is disabled and Record* are no-ops") {
    M::Reset();
    CHECK_FALSE(M::IsEnabled());

    // While disabled, recordings must not leak into the snapshot.
    M::RecordParseTime(42, 1ms);
    M::RecordCacheHit("foo");
    M::RecordCacheMiss("bar");

    auto snap = M::CurrentSnapshot();
    CHECK(snap.parseTimes.empty());
    CHECK(snap.cacheHits.empty());
    CHECK(snap.cacheMisses.empty());
}

TEST_CASE("[Metrics] Enable + record + snapshot accumulates") {
    M::Reset();
    M::Enable(true);
    CHECK(M::IsEnabled());

    for (int i = 0; i < 100; ++i) {
        M::RecordCacheHit("foo");
    }
    for (int i = 0; i < 33; ++i) {
        M::RecordCacheMiss("foo");
    }
    M::RecordParseTime(7, 500us);
    M::RecordParseTime(7, 250us);
    M::RecordParseTime(9, 1ms);

    auto snap = M::CurrentSnapshot();
    CHECK(snap.cacheHits["foo"]   == 100);
    CHECK(snap.cacheMisses["foo"] == 33);
    CHECK(snap.parseTimes[7]      == 750us);
    CHECK(snap.parseTimes[9]      == 1000us);

    // Disable must not wipe accumulated state — only Reset() does.
    M::Enable(false);
    CHECK_FALSE(M::IsEnabled());
    snap = M::CurrentSnapshot();
    CHECK(snap.cacheHits["foo"] == 100);

    M::Reset();
    snap = M::CurrentSnapshot();
    CHECK(snap.parseTimes.empty());
}

TEST_CASE("[Metrics] Thread-safe under concurrent recording") {
    M::Reset();
    M::Enable(true);

    constexpr int kThreads      = 4;
    constexpr int kPerThread    = 5'000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kPerThread; ++i) {
                M::RecordCacheHit("shared");
                M::RecordParseTime(static_cast<uint32_t>(t), 1us);
            }
        });
    }
    for (auto& th : threads) th.join();

    auto snap = M::CurrentSnapshot();
    CHECK(snap.cacheHits["shared"] == kThreads * kPerThread);
    for (int t = 0; t < kThreads; ++t) {
        CHECK(snap.parseTimes[static_cast<uint32_t>(t)]
              == std::chrono::microseconds(kPerThread));
    }

    M::Reset();
    M::Enable(false);
}

TEST_CASE("[Metrics] RecordParseTime is cheap when disabled") {
    // Smoke-only benchmark — guards against accidentally adding a lock or
    // map lookup to the disabled path. The roadmap targets < 5 ns per
    // call; even a debug build comfortably beats a few-hundred-ns budget.
    M::Reset();
    M::Enable(false);

    constexpr int kIters = 1'000'000;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kIters; ++i) {
        M::RecordParseTime(1, 1us);
    }
    auto t1 = std::chrono::steady_clock::now();
    auto perCall = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0) / kIters;

    MESSAGE("[Metrics] disabled RecordParseTime cost ~"
            << perCall.count() << " ns/call");
    // Generous ceiling — function call + relaxed atomic load + branch.
    CHECK(perCall < std::chrono::nanoseconds(500));

    // Recordings during the loop should have been ignored.
    auto snap = M::CurrentSnapshot();
    CHECK(snap.parseTimes.empty());
}
