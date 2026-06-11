#pragma once
#include <string>
#include <filesystem>

#if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <CoreFoundation/CoreFoundation.h>
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

// On macOS, when running from a .app bundle the resources live in
// Contents/Resources/ rather than next to the executable.
// Detect this by checking if the executable dir ends with .app/Contents/MacOS.
inline std::filesystem::path getResourceDir() {
#if defined(__APPLE__)
    auto execDir = getExecutableDir();
    // Check if we're inside a .app bundle: .../Foo.app/Contents/MacOS
    auto contents = execDir.parent_path();           // .../Foo.app/Contents
    auto bundle   = contents.parent_path();          // .../Foo.app
    if (execDir.filename() == "MacOS"
        && contents.filename() == "Contents"
        && bundle.extension() == ".app") {
        return contents / "Resources";
    }
#endif
    // Not in a bundle (or non-Apple): resources are next to the executable
    return getExecutableDir();
}

inline std::string resolvePath(const std::string& relativePath) {
    return (getResourceDir() / relativePath).string();
}

} // namespace PathUtils

