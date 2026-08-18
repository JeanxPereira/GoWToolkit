#include <glad/glad.h>  // Must be before GLFW
#include <GLFW/glfw3.h>

#include "cli/HeadlessGL.h"

#include <Onyx/Rendering/ShaderManager.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace Onyx::Cli {

namespace {

const char* FboStatusName(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:                      return "complete";
        case GL_FRAMEBUFFER_UNDEFINED:                     return "undefined";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         return "incomplete attachment";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "missing attachment";
        case GL_FRAMEBUFFER_UNSUPPORTED:                   return "unsupported format";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:        return "incomplete multisample";
        default:                                           return "unknown";
    }
}

std::string g_glfwError;

void GlfwErrorCallback(int code, const char* desc) {
    g_glfwError = "GLFW error " + std::to_string(code) + ": " +
                  (desc ? desc : "(no description)");
}

} // namespace

HeadlessGL::~HeadlessGL() {
    if (m_window) {
        DestroyFramebuffers();
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_glfwInited) {
        glfwTerminate();
        m_glfwInited = false;
    }
}

bool HeadlessGL::Init(std::string& err) {
    glfwSetErrorCallback(&GlfwErrorCallback);

    if (!glfwInit()) {
        err = "glfwInit failed. " + (g_glfwError.empty()
              ? std::string("No display session? Headless GL still needs one.")
              : g_glfwError);
        return false;
    }
    m_glfwInited = true;

    // Same context the app asks for, so shaders compile identically. macOS
    // caps out at 3.2 core and its GLSL is #version 150; everywhere else the
    // app targets 3.3.
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // The default framebuffer is never drawn to — every pixel goes through the
    // FBO pair below — so ask for the cheapest one the driver will give.
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);

    m_window = glfwCreateWindow(64, 64, "GoWTool headless", nullptr, nullptr);
    if (!m_window) {
        err = "glfwCreateWindow failed. " + (g_glfwError.empty()
              ? std::string("No OpenGL 3.3 core context available.")
              : g_glfwError);
        return false;
    }

    glfwMakeContextCurrent(m_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        err = "gladLoadGLLoader failed — no usable GL function pointers.";
        return false;
    }

    // Viewport3D::InitFBO does this too; the shaders are compiled lazily off it.
    Rendering::ShaderManager::Get().Initialize();
    return true;
}

void HeadlessGL::DestroyFramebuffers() {
    if (m_msaaColor)    { glDeleteTextures(1, &m_msaaColor);       m_msaaColor = 0; }
    if (m_msaaRbo)      { glDeleteRenderbuffers(1, &m_msaaRbo);    m_msaaRbo = 0; }
    if (m_msaaFbo)      { glDeleteFramebuffers(1, &m_msaaFbo);     m_msaaFbo = 0; }
    if (m_resolveColor) { glDeleteTextures(1, &m_resolveColor);    m_resolveColor = 0; }
    if (m_resolveFbo)   { glDeleteFramebuffers(1, &m_resolveFbo);  m_resolveFbo = 0; }
    m_width = m_height = 0;
}

bool HeadlessGL::BeginFrame(int width, int height, std::string& err) {
    if (!m_window) { err = "HeadlessGL::Init was not called"; return false; }
    if (width <= 0 || height <= 0) { err = "invalid frame size"; return false; }

    if (width != m_width || height != m_height) {
        DestroyFramebuffers();

        GLint maxSamples = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        const int samples = m_samples < maxSamples ? m_samples : maxSamples;

        glGenFramebuffers(1, &m_msaaFbo);
        glGenTextures(1, &m_msaaColor);
        glGenRenderbuffers(1, &m_msaaRbo);
        glGenFramebuffers(1, &m_resolveFbo);
        glGenTextures(1, &m_resolveColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaaColor);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8,
                                width, height, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D_MULTISAMPLE, m_msaaColor, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msaaRbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                         GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_msaaRbo);

        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE) {
            err = std::string("MSAA framebuffer incomplete: ") + FboStatusName(st);
            DestroyFramebuffers();
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
        glBindTexture(GL_TEXTURE_2D, m_resolveColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_resolveColor, 0);

        st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE) {
            err = std::string("resolve framebuffer incomplete: ") + FboStatusName(st);
            DestroyFramebuffers();
            return false;
        }

        m_width  = width;
        m_height = height;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFbo);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    return true;
}

bool HeadlessGL::EndFrame(std::vector<uint8_t>& rgbaOut, std::string& err) {
    if (!m_msaaFbo || m_width <= 0) { err = "EndFrame without BeginFrame"; return false; }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFbo);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    std::vector<uint8_t> flipped((size_t)m_width * m_height * 4);
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());

    GLenum e = glGetError();
    if (e != GL_NO_ERROR) {
        err = "glReadPixels reported GL error 0x" + std::to_string(e);
        return false;
    }

    // GL hands back bottom-up; PNG wants top-down.
    const size_t stride = (size_t)m_width * 4;
    rgbaOut.resize(flipped.size());
    for (int y = 0; y < m_height; ++y) {
        const uint8_t* src = flipped.data() + (size_t)(m_height - 1 - y) * stride;
        std::copy(src, src + stride, rgbaOut.data() + (size_t)y * stride);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_MULTISAMPLE);
    return true;
}

bool HeadlessGL::WritePng(const std::filesystem::path& path,
                          int width, int height,
                          const std::vector<uint8_t>& rgba,
                          std::string& err)
{
    if (rgba.size() < (size_t)width * height * 4) {
        err = "pixel buffer smaller than the declared frame";
        return false;
    }
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    if (!stbi_write_png(path.string().c_str(), width, height, 4,
                        rgba.data(), width * 4)) {
        err = "stbi_write_png failed for " + path.string();
        return false;
    }
    return true;
}

} // namespace Onyx::Cli
