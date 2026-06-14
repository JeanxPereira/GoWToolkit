#include "Window/Window.h"

#if defined(__linux__)

#include <GLFW/glfw3.h>

namespace Onyx::App {

// -- Platform-specific GL hints -----------------------------------------------
void Window::configureGLFW() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
}

// -- Pre-window platform setup ------------------------------------------------
void Window::initNative() {
    // No-op on Linux for now
}

// -- Post-window platform setup -----------------------------------------------
void Window::setupNativeWindow() {
    // No-op on Linux for now
}

// -- Per-frame platform hooks -------------------------------------------------
void Window::beginNativeWindowFrame() {}
void Window::endNativeWindowFrame()   {}

} // namespace Onyx::App

#endif
