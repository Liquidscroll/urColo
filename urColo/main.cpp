#ifdef _WIN32
#include <tchar.h>
#include <windows.h>

#include <imgui.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM,
                                                             LPARAM);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
#else
#include <GLFW/glfw3.h>
#endif
#include <print>

#include "Gui.h"
#include "Gui/WindowManager.h"
#include "Logger.h"

int main(int, char **) {

    // start logger
    auto logger = uc::Logger();

#ifdef _WIN32
    WNDCLASSEXW wc = {sizeof(wc), CS_OWNDC,  WndProc,
                      0L,         0L,        GetModuleHandle(nullptr),
                      nullptr,    nullptr,   nullptr,
                      nullptr,    L"urColo", nullptr};
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"urColo",
                                WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720,
                                nullptr, nullptr, wc.hInstance, nullptr);
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    uc::GuiManager gui;
    uc::WindowManager wm;
    wm.init(&gui, hwnd);

    MSG msg;
    bool done = false;
    while (!done) {
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        wm.newFrame();
        wm.render();
    }
    wm.shutdown();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
#else
    if (!glfwInit()) {
        std::print(stderr, "[ERROR]: Failed to initialise GLFW\n");
        return 1;
    }

    const char *glsl_version = "#version 330 core";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    GLFWwindow *wind =
        glfwCreateWindow(1280, 720, "urColo - Hello ImGUI", NULL, NULL);

    if (wind == NULL) {
        std::print(stderr, "[ERROR]: Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    uc::GuiManager gui;
    uc::WindowManager wm;
    wm.init(&gui, wind, glsl_version);

    while (!glfwWindowShouldClose(wind)) {
        glfwPollEvents();
        wm.newFrame();
        wm.render();
    }
    wm.shutdown();

    glfwDestroyWindow(wind);
    glfwTerminate();
#endif

    logger.shutdown();

    return 0;
}
