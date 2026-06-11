#include "Metrics.h"

#include <atomic>

namespace Onyx::Metrics {

namespace {

// Hot-path flag: a relaxed load is enough — recordings should reflect
// the most recent Enable/Disable on a best-effort basis. Strict
// ordering across threads is not required for an instrumentation
// surface.
std::atomic<bool> g_enabled{false};

// All shared state lives behind this mutex. Held only inside the slow
// path (i.e., when metrics are enabled). The locked region is bounded
// (map insertion / increment), so there is no need for a finer-grained
// lock until proven necessary.
std::mutex                                       g_mutex;
std::map<uint32_t, std::chrono::microseconds>    g_parseTimes;
std::map<std::string, uint64_t>                  g_cacheHits;
std::map<std::string, uint64_t>                  g_cacheMisses;

} // anonymous namespace

void Enable(bool on) {
    g_enabled.store(on, std::memory_order_relaxed);
}

bool IsEnabled() {
    return g_enabled.load(std::memory_order_relaxed);
}

void Reset() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_parseTimes.clear();
    g_cacheHits.clear();
    g_cacheMisses.clear();
}

void RecordParseTime(uint32_t typeId, std::chrono::microseconds duration) {
    if (!g_enabled.load(std::memory_order_relaxed)) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_parseTimes[typeId] += duration;
}

void RecordCacheHit(const char* name) {
    if (!g_enabled.load(std::memory_order_relaxed)) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    ++g_cacheHits[name];
}

void RecordCacheMiss(const char* name) {
    if (!g_enabled.load(std::memory_order_relaxed)) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    ++g_cacheMisses[name];
}

Snapshot CurrentSnapshot() {
    std::lock_guard<std::mutex> lock(g_mutex);
    Snapshot s;
    s.parseTimes  = g_parseTimes;
    s.cacheHits   = g_cacheHits;
    s.cacheMisses = g_cacheMisses;
    return s;
}

} // namespace Onyx::Metrics
