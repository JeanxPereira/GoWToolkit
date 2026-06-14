#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <cstdint>
#include <Onyx/App/App.h>
#include <Onyx/Services/AppConfig.h>

namespace Onyx::App {

class Window {
public:
    Window();
    ~Window();

    // Finalizes App init and enters the frame loop. App init is deferred to
    // here (not the constructor) so the executable can inject the game
    // panel/viewer registrar via app() before App::init() runs.
    void run();

    void loop();
    void fullFrame();

    // Called from main() before constructing Window
    static void initNative();

    GLFWwindow* getGLFWwindow() const { return m_window; }

    // Minimal accessor so the executable can install its game-specific
    // registrar onto the App before run()/init().
    App& app() { return m_app; }

    // Config accessors
    bool isBorderless() const { return !m_config.nativeDecorations; }

private:
    // â”€â”€ Platform-specific (implemented per OS in window/platform/) â”€â”€
    void configureGLFW();
    void setupNativeWindow();
    void beginNativeWindowFrame();
    void endNativeWindowFrame();

    // â”€â”€ Lifecycle â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    void initGLFW();
    void initImGui();
    void exitGLFW();
    void exitImGui();

    // â”€â”€ Frame phases â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    void frameBegin();
    void frame();
    void frameEnd();

    // â”€â”€ Optimizations â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    bool shouldRender();   // vtx buffer diff â€” skip GPU when idle
    void unlockFrameRate();

    // â”€â”€ Members â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    GLFWwindow*  m_window   = nullptr;
    App                         m_app;
    Onyx::Services::AppConfig   m_config;

    // Frame rate control
    bool   m_shouldUnlockFrameRate = false;
    double m_fpsUnlockedEndTime    = 0.0;
    bool   m_firstFrame            = true;

    // Vtx buffer diff
    std::vector<uint8_t> m_previousVtxData;
    size_t               m_previousVtxDataSize = 0;

    // Config path
    std::string m_configPath;
};

} // namespace Onyx::App
