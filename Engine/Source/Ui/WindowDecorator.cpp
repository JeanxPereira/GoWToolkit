#include <Onyx/App/WindowDecorator.h>
#include "Ui/TitleBar.h"
#include "Ui/NativeWindow.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Onyx::App {

void WindowDecorator::init(GLFWwindow* window, const char* /*unused*/) {
    // Icon font is now managed centrally by Onyx::Fonts::BuildAtlas()
    NativeWindow::setup(window, borderless);
}

void WindowDecorator::beginFrame(GLFWwindow* window) {
    // 1:1 ImHex: s_titleBarHeight = ImGui::GetCurrentWindowRead()->MenuBarHeight
    // Chamado apÃ³s ImGui::Begin("##HostWindow") para ter o valor correcto
    float h = 0.0f;
    if (GImGui && GImGui->CurrentWindow)
        h = GImGui->CurrentWindow->MenuBarHeight;
    NativeWindow::beginFrame(window, h);
}

bool WindowDecorator::drawTitleBar(GLFWwindow* window, const std::string& title) {
    TitleBar::drawBackDrop();
    return TitleBar::draw(window, title, borderless);
}

} // namespace Onyx::App
