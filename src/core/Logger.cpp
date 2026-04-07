#include "Logger.h"
#include <cstdarg>
#include <ctime>
#include <iostream>

namespace GOW {

Logger& Logger::Get() {
    static Logger instance;
    return instance;
}

void Logger::Log(LogLevel level, const char* fmt, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    time_t rawtime;
    struct tm* timeinfo;
    char timeBuffer[80] = {0};
    
    time(&rawtime);
#ifdef _WIN32
    struct tm timeinfo_s;
    localtime_s(&timeinfo_s, &rawtime);
    timeinfo = &timeinfo_s;
#else
    timeinfo = localtime(&rawtime);
#endif

    if (timeinfo) {
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeinfo);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.push_back({level, std::string(timeBuffer), std::string(buffer)});
    
    // Eco no console padrão (Debug não é impresso)
    if (level == LogLevel::Error) {
        std::cerr << "[" << timeBuffer << "] [ERROR] " << buffer << std::endl;
    } else if (level == LogLevel::Warning) {
        std::cout << "[" << timeBuffer << "] [WARN]  " << buffer << std::endl;
    } else if (level == LogLevel::Info) {
        std::cout << "[" << timeBuffer << "] [INFO]  " << buffer << std::endl;
    }
}

std::vector<LogEntry> Logger::GetEntries() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries;
}

void Logger::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

} // namespace GOW
