#pragma once

// GOW::Metrics — lightweight, opt-in instrumentation surface.
//
// The metrics are disabled by default. While disabled, every recording
// function is a single inlineable branch (a `load(memory_order_relaxed)`
// check) that returns without touching any shared state — the cost on a
// production hot path is negligible.
//
// When `Enable(true)` is called, recordings are routed into thread-safe
// counters guarded by a single mutex. The intended consumer is the test
// suite and the inspector UI; this is *not* a production telemetry
// pipeline.
//
// The header keeps its include surface intentionally tiny so that
// dropping `Metrics::RecordCacheHit` into any translation unit does not
// drag in the rest of the core. `TypeId` is intentionally not pulled in
// here — callers `static_cast<uint32_t>(typeId)` at the call site.

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace GOW::Metrics {

struct Snapshot {
    // Accumulated parse time per TypeId (caller passed as uint32_t).
    std::map<uint32_t, std::chrono::microseconds> parseTimes;
    // Cache hit/miss counters keyed by an arbitrary caller-supplied label.
    std::map<std::string, uint64_t> cacheHits;
    std::map<std::string, uint64_t> cacheMisses;
};

// Default: disabled. Turning it on is cheap; turning it off resets the
// "enabled" flag but leaves accumulated counters in place — call
// `Reset()` to clear them.
void Enable(bool on);
bool IsEnabled();

// Wipes all accumulated counters and parse-time totals. Does not change
// the enabled state.
void Reset();

// Each Record* is a no-op when metrics are disabled. The hot path is a
// single relaxed atomic load of the enabled flag, no allocations.
void RecordParseTime(uint32_t typeId, std::chrono::microseconds duration);
void RecordCacheHit(const char* name);
void RecordCacheMiss(const char* name);

// Returns a copy of the current counters. Safe to call while other
// threads continue to record (acquires the internal mutex briefly).
Snapshot CurrentSnapshot();

} // namespace GOW::Metrics
