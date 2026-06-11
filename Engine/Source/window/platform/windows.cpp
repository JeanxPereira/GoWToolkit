#include "window/Window.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#pragma comment(lib, "dwmapi.lib")

// ── Platform-specific GL hints ──────────────────────────────────────────────
void Window::configureGLFW() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
}

// ── Pre-window platform setup ───────────────────────────────────────────────
void Window::initNative() {
    // No-op on Windows for now
}

// ── Post-window platform setup ──────────────────────────────────────────────
// NOTE: The complex borderless WndProc is handled by NativeWindow_windows.cpp
// which is still active. This method does the basics.
void Window::setupNativeWindow() {
    // NativeWindow::setup() is called from App::init() via the WindowDecorator
    // So we don't duplicate it here — the existing NativeWindow code handles it.
}

// ── Per-frame platform hooks ────────────────────────────────────────────────
void Window::beginNativeWindowFrame() {
    // The existing NativeWindow::beginFrame() handles WS_OVERLAPPEDWINDOW etc.
    // That is called from WindowDecorator::beginFrame() in App::frameBegin().
}

void Window::endNativeWindowFrame() {
    // No-op
}

#endif
