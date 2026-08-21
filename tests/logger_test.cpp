// Renamed from the GOW_LOG_*/LOG_INFO spellings this file was written
// against. Both families still exist under their Onyx names and the mapping is
// exact:
//
//   GOW_LOG_{TRACE,DEBUG,INFO,WARN,ERROR}(cat, fmt, ...)
//       -> ONYX_LOG_*  -- same shape, takes a category
//   LOG_INFO(fmt, ...)
//       -> ONYX_LOGF_* -- printf-style, no category
//
// The two are NOT interchangeable: mapping the printf-style call onto the
// category-taking macro compiles fine and silently turns the format string
// into a category, which is precisely what the "Legacy LOG_INFO" case below
// asserts against (category == "").
//
// This file was excluded from the target during the port on the belief that it
// tested a macro family that never existed here. It tested the one that did.

#include <doctest/doctest.h>

#include <mutex>
#include <string>
#include <vector>

#include <Onyx/Services/Logger.h>

namespace L = Onyx::Services::Log;

namespace {

struct CapturedLine {
    L::Level    level;
    std::string category;
    std::string message;
};

struct CaptureBuffer {
    std::mutex                mutex;
    std::vector<CapturedLine> lines;

    L::SinkFn AsSink() {
        return [this](L::Level lvl, std::string_view cat, std::string_view msg) {
            std::lock_guard<std::mutex> lk(mutex);
            lines.push_back({ lvl, std::string(cat), std::string(msg) });
        };
    }
};

// Helper RAII that wipes any inherited sinks for the duration of a test,
// then restores a clean state afterwards. Tests should not bleed sinks
// into one another.
struct IsolatedLog {
    L::Level previousMin;

    IsolatedLog() : previousMin(L::GetMinLevel()) {
        L::ClearSinks();
        L::SetMinLevel(L::Level::Trace);
    }
    ~IsolatedLog() {
        L::ClearSinks();
        L::SetMinLevel(previousMin);
    }
};

} // anonymous namespace

TEST_CASE("[Logger] FormatLine emits the canonical [LEVEL][cat] msg shape") {
    CHECK(L::FormatLine(L::Level::Info, "test", "hello 42") == "[INFO][test] hello 42");
    CHECK(L::FormatLine(L::Level::Warn, "wad",  "uh oh")    == "[WARN][wad] uh oh");
    CHECK(L::FormatLine(L::Level::Error, "", "boom")        == "[ERROR][] boom");
}

TEST_CASE("[Logger] ONYX_LOG_INFO routes to sinks with formatted message") {
    IsolatedLog iso;

    CaptureBuffer cap;
    L::AddSink(cap.AsSink());

    ONYX_LOG_INFO("test", "hello {}", 42);

    REQUIRE(cap.lines.size() == 1);
    CHECK(cap.lines[0].level    == L::Level::Info);
    CHECK(cap.lines[0].category == "test");
    CHECK(cap.lines[0].message  == "hello 42");

    // Canonical rendered line matches the M0.T5 AC literal:
    CHECK(L::FormatLine(cap.lines[0].level, cap.lines[0].category, cap.lines[0].message)
          == "[INFO][test] hello 42");
}

TEST_CASE("[Logger] SetMinLevel drops records below the threshold") {
    IsolatedLog iso;

    CaptureBuffer cap;
    L::AddSink(cap.AsSink());
    L::SetMinLevel(L::Level::Warn);

    ONYX_LOG_TRACE("test", "trace");
    ONYX_LOG_DEBUG("test", "debug");
    ONYX_LOG_INFO("test",  "info");
    ONYX_LOG_WARN("test",  "warn");
    ONYX_LOG_ERROR("test", "error");

    REQUIRE(cap.lines.size() == 2);
    CHECK(cap.lines[0].level == L::Level::Warn);
    CHECK(cap.lines[1].level == L::Level::Error);

    // Bumping the threshold up at runtime takes effect immediately.
    L::SetMinLevel(L::Level::Error);
    ONYX_LOG_WARN("test", "warn after rethreshold");
    ONYX_LOG_ERROR("test", "error after rethreshold");
    REQUIRE(cap.lines.size() == 3);
    CHECK(cap.lines[2].message == "error after rethreshold");
}

TEST_CASE("[Logger] AddSink/RemoveSink with token controls fan-out") {
    IsolatedLog iso;

    CaptureBuffer cap1, cap2;
    auto t1 = L::AddSink(cap1.AsSink());
    auto t2 = L::AddSink(cap2.AsSink());

    ONYX_LOG_INFO("cat", "both");
    CHECK(cap1.lines.size() == 1);
    CHECK(cap2.lines.size() == 1);

    L::RemoveSink(t1);

    ONYX_LOG_INFO("cat", "only cap2");
    CHECK(cap1.lines.size() == 1);
    CHECK(cap2.lines.size() == 2);

    L::RemoveSink(t2);
    ONYX_LOG_INFO("cat", "no one");
    CHECK(cap1.lines.size() == 1);
    CHECK(cap2.lines.size() == 2);
}

TEST_CASE("[Logger] Legacy LOG_INFO funnels through the new pipeline") {
    IsolatedLog iso;

    CaptureBuffer cap;
    L::AddSink(cap.AsSink());

    ONYX_LOGF_INFO("legacy %d %s", 7, "rocks");

    REQUIRE(cap.lines.size() == 1);
    CHECK(cap.lines[0].level    == L::Level::Info);
    CHECK(cap.lines[0].category == "");
    CHECK(cap.lines[0].message  == "legacy 7 rocks");
}

TEST_CASE("[Logger] Memory ring backs Logger::GetEntries for the UI") {
    IsolatedLog iso;

    Onyx::Services::Logger::Get().Clear();
    ONYX_LOG_INFO("ui", "first");
    ONYX_LOG_WARN("ui", "second");

    auto entries = Onyx::Services::Logger::Get().GetEntries();
    REQUIRE(entries.size() >= 2);
    CHECK(entries[entries.size() - 2].message == "first");
    CHECK(entries[entries.size() - 1].message == "second");
    CHECK(entries[entries.size() - 1].level   == Onyx::Services::LogLevel::Warning);

    Onyx::Services::Logger::Get().Clear();
    CHECK(Onyx::Services::Logger::Get().GetEntries().empty());
}
