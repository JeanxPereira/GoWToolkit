#include <Onyx/Platform/SystemTheme.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Onyx::Platform {

SystemAppearance DetectSystemAppearance() {
    // HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme
    //   DWORD = 0 â†’ apps use dark theme
    //   DWORD = 1 â†’ apps use light theme
    HKEY key;
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &key) != ERROR_SUCCESS) {
        return SystemAppearance::Dark; // pre-Win10 / no registry value
    }
    DWORD value = 0;
    DWORD valueSize = sizeof(value);
    DWORD type = 0;
    LONG status = RegQueryValueExW(
        key, L"AppsUseLightTheme", nullptr, &type,
        reinterpret_cast<LPBYTE>(&value), &valueSize);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_DWORD) {
        return SystemAppearance::Dark;
    }
    return (value == 0) ? SystemAppearance::Dark : SystemAppearance::Light;
}

} // namespace Onyx::Platform

#endif // _WIN32
