#include "UIHelpers.h"
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

#if defined(__APPLE__)
extern "C" {
    const char* macos_openFileDialog(const char* const* extensions, int extCount);
    const char* macos_saveFileDialog(const char* defaultName);
}
#endif

std::string SystemOpenFileDialog() {
#ifdef _WIN32
    OPENFILENAMEA ofn = {};
    char path[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Supported Files (*.wad, *.iso, *.pak)\0*.wad;*.iso;*.pak\0WAD Files\0*.wad\0ISO Files\0*.iso\0All Files\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        return std::string(path);
    }
#elif defined(__APPLE__)
    const char* exts[] = { "wad", "iso", "pak" };
    const char* result = macos_openFileDialog(exts, 3);
    if (result) {
        std::string path(result);
        free(const_cast<char*>(result));
        return path;
    }
#endif
    return "";
}

std::string SystemSaveFileDialog(const std::string& defaultName) {
#ifdef _WIN32
    OPENFILENAMEA ofn = {};
    char path[MAX_PATH] = {};
    strncpy(path, defaultName.c_str(), sizeof(path) - 1);
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameA(&ofn)) {
        return std::string(path);
    }
#elif defined(__APPLE__)
    const char* result = macos_saveFileDialog(defaultName.c_str());
    if (result) {
        std::string path(result);
        free(const_cast<char*>(result));
        return path;
    }
#endif
    return "";
}
