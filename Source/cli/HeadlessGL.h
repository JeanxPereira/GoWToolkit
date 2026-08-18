#pragma once

// An OpenGL context with no visible window, plus the MSAA + resolve FBO pair
// the viewport draws into, plus a PNG writer.
//
// This duplicates ~40 lines of Viewport3D::ResizeFBO because that method is
// private and its owner is an ImGui widget that cannot exist without a docked
// UI. The duplication is confined to framebuffer setup: nothing that decides
// how a pixel looks lives here — that all stays in SceneRenderer, which the
// caller drives directly, so a headless frame and a viewport frame come out of
// the same shaders.
//
// "Headless" here means "no visible window", not "no display". GLFW still
// creates a real window (hidden) against a real driver, so this needs a
// desktop session. That is fine for its purpose — a developer running the CLI
// on the same machine that runs the app — and it is the limit to remember
// before wiring this into anything that runs without a logged-in session.

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct GLFWwindow;

namespace Onyx::Cli {

class HeadlessGL {
public:
    HeadlessGL() = default;
    ~HeadlessGL();

    HeadlessGL(const HeadlessGL&) = delete;
    HeadlessGL& operator=(const HeadlessGL&) = delete;

    /// Creates the hidden window, loads GL, and initialises ShaderManager.
    bool Init(std::string& err);

    /// (Re)allocates the framebuffers at this size and binds the MSAA target.
    /// Leaves GL ready for a draw: viewport set, colour+depth cleared, depth
    /// test and multisample enabled.
    bool BeginFrame(int width, int height, std::string& err);

    /// Resolves MSAA and reads the frame back as tightly packed top-down RGBA.
    bool EndFrame(std::vector<uint8_t>& rgbaOut, std::string& err);

    /// Writes tightly packed top-down RGBA as a PNG.
    static bool WritePng(const std::filesystem::path& path,
                         int width, int height,
                         const std::vector<uint8_t>& rgba,
                         std::string& err);

    int Width()  const { return m_width; }
    int Height() const { return m_height; }

private:
    void DestroyFramebuffers();

    GLFWwindow*  m_window = nullptr;
    bool         m_glfwInited = false;

    unsigned int m_msaaFbo = 0, m_msaaColor = 0, m_msaaRbo = 0;
    unsigned int m_resolveFbo = 0, m_resolveColor = 0;
    int          m_width = 0, m_height = 0;
    int          m_samples = 4;
};

} // namespace Onyx::Cli
