#include "SystemTheme.h"

#if !defined(_WIN32) && !defined(__APPLE__)

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

namespace Onyx::Platform {

namespace {

// Run a short command, read up to 256 bytes from stdout. Returns empty string
// on failure (popen not available, exit failure, etc.).
std::string ShellCapture(const char* cmd) {
    FILE* p = popen(cmd, "r");
    if (!p) return {};
    std::array<char, 256> buf{};
    size_t n = fread(buf.data(), 1, buf.size() - 1, p);
    pclose(p);
    return std::string(buf.data(), n);
}

} // namespace

SystemAppearance DetectSystemAppearance() {
    // GNOME 42+ / KDE 5.25+: org.gnome.desktop.interface color-scheme.
    //   Values: 'default', 'prefer-dark', 'prefer-light'
    std::string out = ShellCapture("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null");
    if (!out.empty()) {
        if (out.find("prefer-dark") != std::string::npos)  return SystemAppearance::Dark;
        if (out.find("prefer-light") != std::string::npos) return SystemAppearance::Light;
    }

    // Fallback: gtk-theme name contains "dark" → Dark.
    out = ShellCapture("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null");
    if (!out.empty()) {
        for (char &c : out) c = (char)tolower((unsigned char)c);
        if (out.find("dark") != std::string::npos) return SystemAppearance::Dark;
    }

    return SystemAppearance::Dark; // safe default
}

} // namespace Onyx::Platform

#endif // !_WIN32 && !__APPLE__
