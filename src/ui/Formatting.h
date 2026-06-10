#pragma once
#include <string>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace Onyx {

inline std::string HashHex(uint64_t hash) {
    std::ostringstream ss;
    ss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

inline std::string FormatBytes(uint64_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

inline std::string FormatNum(uint64_t n) {
    std::string s = std::to_string(n);
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, ",");
    return s;
}

inline bool MatchesFilter(const std::string& name, const char* filter) {
    if (!filter || !filter[0]) return true;
    std::string n = name, f = filter;
    for (auto& c : n) c = (char)tolower(c);
    for (auto& c : f) c = (char)tolower(c);
    return n.find(f) != std::string::npos;
}

} // namespace Onyx
