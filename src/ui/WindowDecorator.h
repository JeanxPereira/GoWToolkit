#pragma once
#include <GLFW/glfw3.h>
#include <string>

// Orquestra NativeWindow + TitleBar
// Chamado uma vez no startup e depois em cada frame
class WindowDecorator {
public:
    bool  borderless = true;

    // Chamar após ImGui::CreateContext() e ANTES do loop
    void init(GLFWwindow* window, const char* codiconTtfPath);

    // Chamar UMA VEZ por frame DENTRO do BeginMainMenuBar / EndMainMenuBar
    // Retorna true se o utilizador clicou em fechar
    bool drawTitleBar(GLFWwindow* window, const std::string& title);

    // Chamar antes de ImGui::NewFrame()
    void beginFrame(GLFWwindow* window);
};
