#include <glad/glad.h>  // Must be before GLFW
#include "window/Window.h"

#include <cfloat>
#include <cstring>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "core/PathUtils.h"
#include "ui/NativeWindow.h"

// ── Globals needed by GLFW callbacks ────────────────────────────────────────
static Window* s_windowInstance = nullptr;

static void glfw_error_callback(int error, const char* desc) {
    fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

// ── Constructor / Destructor ────────────────────────────────────────────────

Window::Window()
    : m_configPath(PathUtils::resolvePath("gowtool.cfg"))
{
    s_windowInstance = this;

    m_config = AppConfig::load(m_configPath);

    initGLFW();
    initImGui();
    setupNativeWindow();

    m_app.init(m_window, &m_config);

    // 1:1 ImHex: live resize via OS refresh callback
    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow*) {
        if (s_windowInstance)
            s_windowInstance->fullFrame();
    });

    glfwSetDropCallback(m_window, [](GLFWwindow*, int, const char**) {});
}

Window::~Window() {
    // Save config before shutdown
    {
        bool maximized = glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED);
        m_config.maximized = maximized;
        if (!maximized) {
            glfwGetWindowPos(m_window, &m_config.windowX, &m_config.windowY);
            glfwGetWindowSize(m_window, &m_config.windowW, &m_config.windowH);
        }

        size_t len = 0;
        const char* iniString = ImGui::SaveIniSettingsToMemory(&len);
        if (iniString && len > 0) {
            m_config.imguiIniState = std::string(iniString, len);
        }

        m_config.save(m_configPath);
    }

    exitImGui();
    exitGLFW();

    s_windowInstance = nullptr;
}

// ── initGLFW ────────────────────────────────────────────────────────────────

void Window::initGLFW() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        std::exit(-1);
    }

    // Delegate platform-specific GL hints
    configureGLFW();

    m_window = glfwCreateWindow(m_config.windowW, m_config.windowH,
                                "GoWTool", nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        fprintf(stderr, "Failed to create GLFW window\n");
        std::exit(-1);
    }

    glfwSetWindowPos(m_window, m_config.windowX, m_config.windowY);
    if (m_config.maximized)
        glfwMaximizeWindow(m_window);

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize OpenGL loader\n");
        std::exit(-1);
    }

    // Store 'this' for callbacks
    glfwSetWindowUserPointer(m_window, this);
}

// ── initImGui ───────────────────────────────────────────────────────────────

void Window::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr; // No automatic .ini file

    // Load layout from saved config
    if (!m_config.imguiIniState.empty()) {
        ImGui::LoadIniSettingsFromMemory(m_config.imguiIniState.c_str(),
                                          m_config.imguiIniState.size());
    }

    ImGui::StyleColorsDark();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    m_config.applyAccent();
    if (m_config.uiScale != 1.0f)
        ImGui::GetStyle().ScaleAllSizes(m_config.uiScale);
    io.FontGlobalScale = m_config.fontScale;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#if defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
}

// ── exitImGui / exitGLFW ────────────────────────────────────────────────────

void Window::exitImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Window::exitGLFW() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_window = nullptr;
}

// ── loop ────────────────────────────────────────────────────────────────────

void Window::loop() {
    while (!glfwWindowShouldClose(m_window)) {
        // Smart event handling: poll aggressively when active, idle otherwise
        if (m_firstFrame) {
            glfwPollEvents();
            m_firstFrame = false;
        } else {
            bool active = ImGui::IsMouseDown(ImGuiMouseButton_Left)
                       || ImGui::IsAnyItemActive()
                       || ImGui::IsKeyDown(ImGuiMod_Ctrl)
                       || m_shouldUnlockFrameRate
                       || (ImGui::GetPlatformIO().Viewports.Size > 1);

#if defined(__APPLE__)
            active = active || NativeWindow::macosIsWindowBeingResized(m_window);
#endif

            if (active) {
                glfwPollEvents();
                m_shouldUnlockFrameRate = false;
            } else {
                glfwWaitEventsTimeout(1.0 / 15.0); // ~15 FPS idle (down from 30)
            }
        }

#if defined(__APPLE__)
        // Suppress hover effects during live resize (ImHex pattern)
        if (NativeWindow::macosIsWindowBeingResized(m_window))
            ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
#endif

        fullFrame();

        if (m_app.wantClose())
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

// ── fullFrame — crash-protected frame ───────────────────────────────────────

void Window::fullFrame() {
    if (!m_window) return;

    static uint32_t crashWatchdog = 0;

#if !defined(DEBUG) && !defined(_DEBUG)
    try {
#endif
        frameBegin();
        frame();
        frameEnd();
#if !defined(DEBUG) && !defined(_DEBUG)
        crashWatchdog = 0;
    } catch (...) {
        if (++crashWatchdog > 10) std::abort();
        ImGui::EndFrame();
        fprintf(stderr, "Exception caught in fullFrame(), watchdog=%u\n", crashWatchdog);
    }
#endif
}

// ── frameBegin ──────────────────────────────────────────────────────────────

void Window::frameBegin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    beginNativeWindowFrame();
}

// ── frame ───────────────────────────────────────────────────────────────────

void Window::frame() {
    m_app.frame();
}

// ── frameEnd ────────────────────────────────────────────────────────────────

void Window::frameEnd() {
    m_app.frameEnd();

    endNativeWindowFrame();

    ImGui::Render();

    // Vertex buffer diff — only do GPU work if something changed
    if (shouldRender()) {
        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }

    // Viewport windows (external OS windows) must ALWAYS be updated and
    // rendered, independent of the main window's shouldRender() result.
    // Otherwise dragging/interacting with a floating window causes flickering
    // because the external viewport is not redrawn when the main vtx buffer
    // hasn't changed.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

// ── shouldRender — vtx buffer diff (zero GPU idle) ──────────────────────────

bool Window::shouldRender() {
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData) return true;

    size_t totalSize = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++)
        totalSize += drawData->CmdLists[n]->VtxBuffer.Size * sizeof(ImDrawVert);

    // Also check multi-viewport data
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        for (int i = 1; i < ImGui::GetPlatformIO().Viewports.Size; i++) {
            ImGuiViewport* vp = ImGui::GetPlatformIO().Viewports[i];
            if (vp->DrawData) {
                for (int n = 0; n < vp->DrawData->CmdListsCount; n++)
                    totalSize += vp->DrawData->CmdLists[n]->VtxBuffer.Size * sizeof(ImDrawVert);
            }
        }
    }

    if (totalSize != m_previousVtxDataSize) {
        m_previousVtxDataSize = totalSize;
        m_previousVtxData.resize(totalSize);
        // Copy all current data
        size_t offset = 0;
        for (int n = 0; n < drawData->CmdListsCount; n++) {
            const auto& buf = drawData->CmdLists[n]->VtxBuffer;
            size_t sz = buf.Size * sizeof(ImDrawVert);
            std::memcpy(m_previousVtxData.data() + offset, buf.Data, sz);
            offset += sz;
        }
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            for (int i = 1; i < ImGui::GetPlatformIO().Viewports.Size; i++) {
                ImGuiViewport* vp = ImGui::GetPlatformIO().Viewports[i];
                if (vp->DrawData) {
                    for (int n = 0; n < vp->DrawData->CmdListsCount; n++) {
                        const auto& buf = vp->DrawData->CmdLists[n]->VtxBuffer;
                        size_t sz = buf.Size * sizeof(ImDrawVert);
                        std::memcpy(m_previousVtxData.data() + offset, buf.Data, sz);
                        offset += sz;
                    }
                }
            }
        }
        return true;
    }

    // Compare buffers
    size_t offset = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const auto& buf = drawData->CmdLists[n]->VtxBuffer;
        size_t sz = buf.Size * sizeof(ImDrawVert);
        if (std::memcmp(m_previousVtxData.data() + offset, buf.Data, sz) != 0) {
            std::memcpy(m_previousVtxData.data() + offset, buf.Data, sz);
            // Copy the rest too for consistency
            offset += sz;
            for (int m = n + 1; m < drawData->CmdListsCount; m++) {
                const auto& b2 = drawData->CmdLists[m]->VtxBuffer;
                size_t s2 = b2.Size * sizeof(ImDrawVert);
                std::memcpy(m_previousVtxData.data() + offset, b2.Data, s2);
                offset += s2;
            }
            return true;
        }
        offset += sz;
    }

    // Check multi-viewport buffers
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        for (int i = 1; i < ImGui::GetPlatformIO().Viewports.Size; i++) {
            ImGuiViewport* vp = ImGui::GetPlatformIO().Viewports[i];
            if (vp->DrawData) {
                for (int n = 0; n < vp->DrawData->CmdListsCount; n++) {
                    const auto& buf = vp->DrawData->CmdLists[n]->VtxBuffer;
                    size_t sz = buf.Size * sizeof(ImDrawVert);
                    if (std::memcmp(m_previousVtxData.data() + offset, buf.Data, sz) != 0) {
                        std::memcpy(m_previousVtxData.data() + offset, buf.Data, sz);
                        return true;
                    }
                    offset += sz;
                }
            }
        }
    }

    return false;
}

// ── unlockFrameRate ─────────────────────────────────────────────────────────

void Window::unlockFrameRate() {
    glfwPostEmptyEvent();
    m_shouldUnlockFrameRate = true;
}
