#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace GOW {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry {
    LogLevel level;
    std::string time;
    std::string message;
};

class Logger {
public:
    static Logger& Get();
    
    void Log(LogLevel level, const char* fmt, ...);
    
    // Para consumo da UI
    std::vector<LogEntry> GetEntries() const;
    void Clear();

private:
    Logger() = default;
    mutable std::mutex m_mutex;
    std::vector<LogEntry> m_entries;
};

} // namespace GOW

// Macros utilitárias globais
#define LOG_DEBUG(...) GOW::Logger::Get().Log(GOW::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  GOW::Logger::Get().Log(GOW::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)  GOW::Logger::Get().Log(GOW::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERR(...)   GOW::Logger::Get().Log(GOW::LogLevel::Error, __VA_ARGS__)
