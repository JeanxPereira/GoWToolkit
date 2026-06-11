#include <glad/glad.h>  // Must be before GLFW
#include "Window/Window.h"

#include <cfloat>
#include <cstring>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "Core/PathUtils.h"
#include "Core/TaskManager.h"
#include "Core/ThemeManager.h"
#include "Core/ScaleManager.h"
#include "Core/FontManager.h"
#include "Core/Events.h"
#include "Ui/NativeWindow.h"

// â”€â”€ Globals needed by GLFW callbacks â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static Window* s_windowInstance = nullptr;

static void glfw_error_callback(int error, const char* desc) {
    // GLFW_PLATFORM_ERROR (65544) "Cannot query workarea without screen" is a
    // known benign error that occurs on macOS when running under a remote
    // desktop session (TeamViewer, VNC, RDP, or any virtual display) instead
    // of a real physical NSScreen.  The Cocoa backend queries the monitor's
    // usable area during window creation/positioning; that call fails
    // gracefully on a virtual display â€” the window still opens and renders
    // correctly, so the message is just noise.  All other errors are logged.
#if defined(__APPLE__)
    if (error == GLFW_PLATFORM_ERROR && desc && strstr(desc, "workarea"))
        return;
#endif
    fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

// â”€â”€ Constructor / Destructor â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

Window::Window()
    : m_configPath(PathUtils::resolvePath("gowtool.gtkc"))
{
    s_windowInstance = this;

    m_config = AppConfig::load(m_configPath);
    AppConfig::SetInstance(&m_config);

    initGLFW();
    initImGui();
    setupNativeWindow();

    // Initialize core systems
    Onyx::TaskManager::init();
}

// â”€â”€ run â€” finalize App and enter the frame loop â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// App init is deferred out of the constructor so the executable can inject the
// game panel/viewer registrar (via app()) before App::init() â€” which is what
// invokes the registrar â€” runs.

void Window::run() {
    m_app.init(m_window, &m_config);

    // 1:1 ImHex: live resize via OS refresh callback
    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow*) {
        if (s_windowInstance)
            s_windowInstance->fullFrame();
    });

    glfwSetDropCallback(m_window, [](GLFWwindow*, int, const char**) {});

    loop();
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

    // Shutdown core systems
    EventShutdown::post();
    Onyx::EventManager::clear();
    Onyx::TaskManager::exit();

    exitImGui();
    exitGLFW();

    s_windowInstance = nullptr;
}

// â”€â”€ initGLFW â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::initGLFW() {
    glfwSetErrorCallback(glfw_error_callback);

#if defined(__APPLE__)
    // Prevent the Cocoa backend from trying to install a global application
    // menu bar before a real screen is available.  On virtual/remote displays
    // (TeamViewer, VNC, headless CI) this avoids spurious platform errors
    // during glfwInit() and subsequent monitor queries.
    glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_FALSE);
#endif

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        std::exit(-1);
    }

    // Delegate platform-specific GL hints
    configureGLFW();

    m_window = glfwCreateWindow(m_config.windowW, m_config.windowH,
                                "God Of War Toolkit", nullptr, nullptr);
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

    // Track window attributes dynamically since macOS shutdown can miss late bounds queries
    glfwSetWindowPosCallback(m_window, [](GLFWwindow* window, int xpos, int ypos) {
        if (!glfwGetWindowAttrib(window, GLFW_MAXIMIZED)) {
            if (Window* w = (Window*)glfwGetWindowUserPointer(window)) {
                w->m_config.windowX = xpos;
                w->m_config.windowY = ypos;
            }
        }
    });

    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        if (!glfwGetWindowAttrib(window, GLFW_MAXIMIZED)) {
            if (Window* w = (Window*)glfwGetWindowUserPointer(window)) {
                w->m_config.windowW = width;
                w->m_config.windowH = height;
            }
        }
    });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize OpenGL loader\n");
        std::exit(-1);
    }

    // Store 'this' for callbacks
    glfwSetWindowUserPointer(m_window, this);
}

// â”€â”€ initImGui â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
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

    // Apply centralized accent-derived theme (replaces StyleColorsDark + applyAccent)
    Onyx::Theme::ApplyTheme(m_config.getAccent(),
                           (Onyx::Theme::ThemeMode)m_config.themeMode);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Initialize centralized scale system with OS native DPI
    float nativeScale = 1.0f;
#if defined(__APPLE__)
    {
        float xscale = 1.0f, yscale = 1.0f;
        glfwGetWindowContentScale(m_window, &xscale, &yscale);
        nativeScale = xscale;
    }
#endif
    Onyx::Scale::Init(m_config.uiScale, nativeScale);
    Onyx::Scale::ApplyStyleScale(m_config.uiScale);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#if defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
}

// â”€â”€ exitImGui / exitGLFW â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::exitImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void Window::exitGLFW() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_window = nullptr;
}

// â”€â”€ loop â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::loop() {
    while (!glfwWindowShouldClose(m_window)) {
        // Smart event handling: poll aggressively when active, idle otherwise
        if (m_firstFrame) {
            glfwPollEvents();
            m_firstFrame = false;
        } else {
            bool active = ImGui::IsMouseDown(ImGuiMouseButton_Left)
                       || ImGui::IsMouseDown(ImGuiMouseButton_Right)
                       || ImGui::IsMouseDown(ImGuiMouseButton_Middle)
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

// â”€â”€ fullFrame â€” crash-protected frame â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€ frameBegin â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::frameBegin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Drive smooth color transitions (preset switches with ease-out)
    Onyx::Theme::UpdateTransition();

    beginNativeWindowFrame();
}

// â”€â”€ frame â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::frame() {
    m_app.frame();
}

// â”€â”€ frameEnd â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::frameEnd() {
    endNativeWindowFrame();

    ImGui::Render();

    // Always render: shouldRender() only diffs ImGui vertex buffers, which doesn't
    // detect FBO texture content changes (same texture ID, new pixels). Always
    // presenting is safe because the event-wait system already limits idle FPS.
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData) {
            int w, h;
            glfwGetFramebufferSize(m_window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(drawData);
            glfwSwapBuffers(m_window);
        }
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

    // Font rebuild MUST happen after all rendering is complete.
    // Rebuilding the atlas before Render() invalidates the font texture
    // that the current frame's draw commands reference.
    if (Onyx::Fonts::IsPendingRebuild()) {
        Onyx::Fonts::UploadAtlas();
    }

    m_app.frameEnd();
}

// â”€â”€ shouldRender â€” vtx buffer diff (zero GPU idle) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€ unlockFrameRate â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Window::unlockFrameRate() {
    glfwPostEmptyEvent();
    m_shouldUnlockFrameRate = true;
}
