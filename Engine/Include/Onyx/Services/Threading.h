#pragma once

// Threading — small helpers for asserting thread affinity.
//
// `MarkMainThread()` is called once at process entry to record the
// std::thread::id of the main thread. Any code that later wants to
// claim "this must run on the UI/main thread" can guard itself with
// `ASSERT_MAIN_THREAD()`, which is a hard assertion in debug builds
// and disappears in release. Use this aggressively in UI / OpenGL
// call paths once we start enforcing the rule.
//
// M0.T7 only wires the primitives; the guards themselves will be
// added in later milestones, file-by-file.

#include <cassert>

namespace Onyx::Threading {

// Records the current thread as the "main" thread. Subsequent calls
// overwrite the previously stored id — typically called exactly once
// from `main.cpp` before any threads are spawned.
void MarkMainThread();

// True when the calling thread matches the last MarkMainThread() id.
// Returns false (without asserting) if MarkMainThread was never
// called yet, so tests can probe both states without side-effects.
bool IsMainThread();

} // namespace Onyx::Threading

// In a NDEBUG build this expands to nothing — no function call, no
// branch, no symbol reference. `assert(IsMainThread())` is the right
// idiom for a debug-only invariant.
#ifdef NDEBUG
#  define ASSERT_MAIN_THREAD() ((void)0)
#else
#  define ASSERT_MAIN_THREAD() assert(::Onyx::Threading::IsMainThread())
#endif
