#include "Threading.h"

#include <atomic>
#include <thread>

namespace Onyx::Threading {

namespace {

// `std::thread::id` is trivially copyable but not trivially atomic on
// every platform. The id only changes during `MarkMainThread()` (in
// practice once, at startup) so a relaxed atomic flag plus a plain
// id is enough: producer publishes the id, then the flag; consumers
// read the flag, then the id. Pairs use acquire/release to keep the
// id visible on the consumer side.
std::atomic<bool>     g_marked{false};
std::thread::id       g_mainId{};

} // anonymous namespace

void MarkMainThread() {
    g_mainId = std::this_thread::get_id();
    g_marked.store(true, std::memory_order_release);
}

bool IsMainThread() {
    if (!g_marked.load(std::memory_order_acquire)) return false;
    return std::this_thread::get_id() == g_mainId;
}

} // namespace Onyx::Threading
