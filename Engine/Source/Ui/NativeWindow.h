#pragma once

#if defined(_WIN32)
    #ifndef GOWTOOL_OS_WINDOWS
    #define GOWTOOL_OS_WINDOWS
    #endif
#elif defined(__APPLE__)
    #ifndef GOWTOOL_OS_MACOS
    #define GOWTOOL_OS_MACOS
    #endif
#elif defined(__linux__)
    #ifndef GOWTOOL_OS_LINUX
    #define GOWTOOL_OS_LINUX
    #endif
#endif

#include <GLFW/glfw3.h>

namespace Onyx::App::NativeWindow {
    void setFullFrameCallback(void(*cb)());
    void setup(GLFWwindow* window, bool borderless);
    void beginFrame(GLFWwindow* window, float titleBarHeight);
    void endFrame(GLFWwindow* window);

#ifdef GOWTOOL_OS_MACOS
    void macosSetWindowMovable(GLFWwindow* window, bool movable);
    void macosHandleTitlebarDoubleClick(GLFWwindow* window);
    bool macosIsWindowBeingResized(GLFWwindow* window);
    bool macosIsFullScreen(GLFWwindow* window);
#endif
}
