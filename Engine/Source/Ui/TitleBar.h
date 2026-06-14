#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <string>

namespace Onyx::App {

// Réplica do ImGuiExt::TitleBarButton do ImHex
// Button sem borda com ItemSpacing.x = 0, para uso na titlebar
bool TitleBarButton(const char* label, ImVec2 size);

namespace TitleBar {

    // Estado
    extern bool  showTitleBarBackDrop;
    extern float backdropAlpha;   // 0.0–1.0

    // Desenha o backdrop (shadow circle) — chamar dentro do BeginMainMenuBar
    void drawBackDrop();

    // Desenha os botões min/max/close + título centrado
    // Chamar dentro do BeginMainMenuBar, depois do menu
    // Retorna true se a janela deve fechar
    bool draw(GLFWwindow* window, const std::string& title, bool borderless);

} // namespace TitleBar

} // namespace Onyx::App
