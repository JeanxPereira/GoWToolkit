#pragma once

// Structured logging surface for GoWToolkit.
//
// New (M0.T5) call sites use the category-aware macros below and produce
// output via a configurable chain of sinks:
//
//   GOW_LOG_INFO("wad", "loaded {} entries", count);
//
// The default stderr sink renders one line per record in the canonical
// shape `[LEVEL][category] message`. Additional sinks (file, in-memory,
// custom) attach via `Log::AddSink` and detach via `Log::RemoveSink`. A
// rotating file sink can be installed once with
// `Log::InstallRotatingFileSink(path, maxBytes, rotations)`.
//
// Legacy printf-style call sites (`LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`,
// `LOG_ERR`) and the older `Onyx::Logger` facade (with `LogEntry`,
// `GetEntries`, `Clear`) are retained so existing callers (StatusBar,
// many parser TUs) keep working while we migrate the codebase
// category-by-category. Both legacy and new paths funnel through the
// same sink chain, so the UI sees a unified log stream.

#include <cstdarg>
#include <cstdint>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace Onyx {

// ── Legacy logging facade (still used by ~250 call sites) ─────────────────
enum class LogLevel { Debug, Info, Warning, Error };

struct LogEntry {
    LogLevel    level;
    std::string time;
    std::string message;
};

class Logger {
public:
    static Logger& Get();

    // printf-style legacy entrypoint — routes into Log::LogString.
    void Log(LogLevel level, const char* fmt, ...);

    // UI-side accessors. Backed by an always-installed in-memory sink.
    std::vector<LogEntry> GetEntries() const;
    void Clear();

private:
    Logger() = default;
};

// ── New structured surface ────────────────────────────────────────────────
namespace Log {

enum class Level { Trace, Debug, Info, Warn, Error };

const char* LevelName(Level lvl); // "TRACE" .. "ERROR"

using SinkFn    = std::function<void(Level, std::string_view category, std::string_view message)>;
using SinkToken = uint64_t;

// Runtime minimum level. Records below this are dropped before reaching
// any sink. Default: `Level::Info`.
void  SetMinLevel(Level lvl);
Level GetMinLevel();

// Returns a token usable with `RemoveSink`.
SinkToken AddSink(SinkFn sink);
void      RemoveSink(SinkToken token);
void      ClearSinks();

// Convenience installers for the built-in sinks. They are not installed
// automatically except for the in-memory sink, which is always present
// for the legacy `Logger::GetEntries()` accessor.
SinkToken InstallStderrSink();
SinkToken InstallRotatingFileSink(const std::string& path,
                                  size_t maxBytes   = 5u * 1024 * 1024,
                                  size_t rotations  = 3);

// Renders the canonical line shape used by the stderr/file sinks.
// Exposed so tests can verify the exact format without scraping stderr.
std::string FormatLine(Level lvl, std::string_view category, std::string_view message);

// Low-level emit. `Log::Log` (below) is the typed entry point preferred
// in production code.
void LogString(Level lvl, std::string_view category, std::string_view message);

template<typename... Args>
inline void Log(Level lvl, std::string_view category,
                std::format_string<Args...> fmtStr, Args&&... args) {
    if (lvl < GetMinLevel()) return; // fast filter; sinks repeat the check
    LogString(lvl, category, std::format(fmtStr, std::forward<Args>(args)...));
}

} // namespace Log

} // namespace Onyx

// ── New category-aware macros (preferred) ────────────────────────────────
#define GOW_LOG_TRACE(cat, ...) ::Onyx::Log::Log(::Onyx::Log::Level::Trace, (cat), __VA_ARGS__)
#define GOW_LOG_DEBUG(cat, ...) ::Onyx::Log::Log(::Onyx::Log::Level::Debug, (cat), __VA_ARGS__)
#define GOW_LOG_INFO(cat, ...)  ::Onyx::Log::Log(::Onyx::Log::Level::Info,  (cat), __VA_ARGS__)
#define GOW_LOG_WARN(cat, ...)  ::Onyx::Log::Log(::Onyx::Log::Level::Warn,  (cat), __VA_ARGS__)
#define GOW_LOG_ERROR(cat, ...) ::Onyx::Log::Log(::Onyx::Log::Level::Error, (cat), __VA_ARGS__)

// ── Legacy printf-style macros (no category) ─────────────────────────────
#define LOG_DEBUG(...) ::Onyx::Logger::Get().Log(::Onyx::LogLevel::Debug,   __VA_ARGS__)
#define LOG_INFO(...)  ::Onyx::Logger::Get().Log(::Onyx::LogLevel::Info,    __VA_ARGS__)
#define LOG_WARN(...)  ::Onyx::Logger::Get().Log(::Onyx::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERR(...)   ::Onyx::Logger::Get().Log(::Onyx::LogLevel::Error,   __VA_ARGS__)
