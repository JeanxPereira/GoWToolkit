#include "Ui/NativeWindow.h"

#if defined(GOWTOOL_OS_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "imgui.h"
#include "imgui_internal.h"

#pragma comment(lib, "dwmapi.lib")

namespace NativeWindow {

static LONG_PTR  s_oldWndProc    = 0;
static float     s_titleBarHeight = 0.0f;
static GLFWwindow* s_glfwWindow  = nullptr;

// â”€â”€ WM_SETCURSOR: repassa cursores ImGui â†’ Win32 â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void handleSetCursor() {
    switch (ImGui::GetMouseCursor()) {
        case ImGuiMouseCursor_Arrow:      SetCursor(LoadCursor(nullptr, IDC_ARROW));    break;
        case ImGuiMouseCursor_Hand:        SetCursor(LoadCursor(nullptr, IDC_HAND));     break;
        case ImGuiMouseCursor_ResizeEW:    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));   break;
        case ImGuiMouseCursor_ResizeNS:    SetCursor(LoadCursor(nullptr, IDC_SIZENS));   break;
        case ImGuiMouseCursor_ResizeNWSE:  SetCursor(LoadCursor(nullptr, IDC_SIZENWSE)); break;
        case ImGuiMouseCursor_ResizeNESW:  SetCursor(LoadCursor(nullptr, IDC_SIZENESW)); break;
        case ImGuiMouseCursor_ResizeAll:   SetCursor(LoadCursor(nullptr, IDC_SIZEALL));  break;
        case ImGuiMouseCursor_NotAllowed:  SetCursor(LoadCursor(nullptr, IDC_NO));       break;
        case ImGuiMouseCursor_TextInput:   SetCursor(LoadCursor(nullptr, IDC_IBEAM));    break;
        default: break;
    }
}

// â”€â”€ WndProc borderless â€” 1:1 com ImHex â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Forward declaration â€” implementado em main.cpp via glfwSetWindowUserPointer
static void (*s_fullFrameCallback)() = nullptr;

static LRESULT CALLBACK borderlessWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

        case WM_MOVE:
            // 1:1 ImHex: fullFrame() durante move para evitar ghosting
            if (s_fullFrameCallback) s_fullFrameCallback();
            break;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                handleSetCursor();
                return TRUE;
            }
            return CallWindowProcW(reinterpret_cast<WNDPROC>(s_oldWndProc), hwnd, uMsg, wParam, lParam);

        case WM_NCACTIVATE:
        case WM_NCPAINT:
            // obrigatÃ³rio para Aero Snap
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);

        case WM_NCCALCSIZE: {
            if (wParam == TRUE && lParam != 0) {
                NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                RECT& rect  = params->rgrc[0];
                RECT  client = rect;
                
                CallWindowProcW(reinterpret_cast<WNDPROC>(s_oldWndProc), hwnd, uMsg, wParam, lParam);
                
                if (IsZoomed(hwnd)) {
                    WINDOWINFO wi = { sizeof(wi) };
                    GetWindowInfo(hwnd, &wi);
                    rect = {
                        client.left   + (LONG)wi.cyWindowBorders,
                        client.top    + (LONG)wi.cyWindowBorders,
                        client.right  - (LONG)wi.cyWindowBorders,
                        client.bottom - (LONG)wi.cyWindowBorders + 1
                    };
                } else {
                    rect = client;
                }
                return WVR_REDRAW;
            }
            break;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_WINDOWPOSCHANGING:
            reinterpret_cast<LPWINDOWPOS>(lParam)->flags |= SWP_NOCOPYBITS;
            break;

        case WM_NCHITTEST: {
            POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            // Fullscreen â†’ tudo cliente
            if (s_glfwWindow && glfwGetWindowMonitor(s_glfwWindow) != nullptr)
                return HTCLIENT;

            RECT window;
            if (!GetWindowRect(hwnd, &window))
                return HTNOWHERE;

            // Tamanho da borda (1:1 ImHex â€” sem DPI scale porque GLFW jÃ¡ escala)
            const POINT border {
                GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER),
                GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)
            };

            constexpr int RegionClient = 0b0000;
            constexpr int RegionLeft   = 0b0001;
            constexpr int RegionRight  = 0b0010;
            constexpr int RegionTop    = 0b0100;
            constexpr int RegionBottom = 0b1000;

            const int result =
                RegionLeft   * (cursor.x <  (window.left   + border.x)) |
                RegionRight  * (cursor.x >= (window.right  - border.x)) |
                RegionTop    * (cursor.y <  (window.top    + border.y)) |
                RegionBottom * (cursor.y >= (window.bottom - border.y));

            // 1:1 ImHex: "If the mouse is hovering over any button, disable resize"
            if (result != 0 && ImGui::IsAnyItemHovered())
                break;

            // 1:1 ImHex: popup aberto
            if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
                return (result == RegionClient) ? HTCLIENT : HTCAPTION;
            }

            // 1:1 ImHex: nome da janela hovered
            const char* hoveredWindowName =
                (ImGui::GetCurrentContext() && GImGui->HoveredWindow)
                ? GImGui->HoveredWindow->Name : "";

            // switch 1:1 com ImHex
            switch (result) {
                case RegionLeft:                   return HTLEFT;
                case RegionRight:                  return HTRIGHT;
                case RegionTop:                    return HTTOP;
                case RegionBottom:                 return HTBOTTOM;
                case RegionTop    | RegionLeft:    return HTTOPLEFT;
                case RegionTop    | RegionRight:   return HTTOPRIGHT;
                case RegionBottom | RegionLeft:    return HTBOTTOMLEFT;
                case RegionBottom | RegionRight:   return HTBOTTOMRIGHT;
                case RegionClient:
                default:
                    // 1:1 ImHex: s_titleBarHeight * 2
                    // nomes: "##HostWindow" (nosso) e "GoWToolDockSpace"
                    if (cursor.y < (window.top + (LONG)(s_titleBarHeight * 2.0f))) {
                        if (strcmp(hoveredWindowName, "##HostWindow") == 0 ||
                            strcmp(hoveredWindowName, "GoWToolDockSpace") == 0)
                        {
                            if (!ImGui::IsAnyItemHovered())
                                return HTCAPTION;
                        }
                    }
                    break;
            }
            break;
        }

        default: break;
    }

    return CallWindowProcW(reinterpret_cast<WNDPROC>(s_oldWndProc), hwnd, uMsg, wParam, lParam);
}

// â”€â”€ API pÃºblica â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void setFullFrameCallback(void(*cb)()) {
    s_fullFrameCallback = cb;
}

void setup(GLFWwindow* window, bool borderless) {
    s_glfwWindow = window;
    HWND hwnd = glfwGetWin32Window(window);

    if (borderless) {
        s_oldWndProc = SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(borderlessWindowProc));

        MARGINS m = { 1, 1, 1, 1 };
        DwmExtendFrameIntoClientArea(hwnd, &m);

        DWORD attr = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &attr, sizeof(attr));

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE);

        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    } else {
        // modo com decoraÃ§Ã£o nativa â€” sem WndProc custom
    }
}

void beginFrame(GLFWwindow* window, float titleBarHeight) {
    if (titleBarHeight > 0.0f)
        s_titleBarHeight = titleBarHeight;

    HWND hwnd = glfwGetWin32Window(window);

    // 1:1 ImHex: remove WS_POPUP (que o GLFW adiciona) para que
    // WS_OVERLAPPEDWINDOW activo permita resize nativo.
    // Sem isto o resize NÃƒO FUNCIONA em modo borderless.
    {
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style |= WS_OVERLAPPEDWINDOW;
        style &= ~WS_POPUP;
        SetWindowLong(hwnd, GWL_STYLE, style);
    }

    // 1:1 ImHex: WS_EX_COMPOSITED elimina flickering ao redimensionar
    {
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_COMPOSITED;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
}

void endFrame(GLFWwindow*) {}

} // namespace NativeWindow
#endif
