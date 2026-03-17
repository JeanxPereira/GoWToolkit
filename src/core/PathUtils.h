#pragma once
#include <string>
#include <filesystem>

#if defined(__APPLE__)
    #include <mach-o/dyld.h>
#elif defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <climits>
#endif

namespace PathUtils {

inline std::filesystem::path getExecutableDir() {
#if defined(__APPLE__)
    char buf[1024];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return std::filesystem::canonical(buf).parent_path();
    }
    // Fallback: try larger buffer
    std::string large(size, '\0');
    if (_NSGetExecutablePath(large.data(), &size) == 0) {
        return std::filesystem::canonical(large.c_str()).parent_path();
    }
#elif defined(_WIN32)
    wchar_t buf[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return std::filesystem::path(buf).parent_path();
    }
#elif defined(__linux__)
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return std::filesystem::path(buf).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

inline std::string resolvePath(const std::string& relativePath) {
    return (getExecutableDir() / relativePath).string();
}

} // namespace PathUtils
