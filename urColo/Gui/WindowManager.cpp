#include "WindowManager.h"
#include "../Gui.h"
#include "../Model.h"
#include "../PaletteGenerator.h"
#include <imgui.h>
#ifndef _WIN32
#include <imgui_impl_glfw.h>
#else
#include <imgui_impl_win32.h>
#include <windows.h>
#endif
#include <imgui_impl_opengl3.h>
#ifdef _WIN32
#include <GL/GL.h>
#else
#include <GL/gl.h>
#endif
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <print>

namespace uc {

void WindowManager::init(GuiManager *gui,
#ifdef _WIN32
                         HWND hwnd, const char *glsl_version)
#else
                         GLFWwindow *wind, const char *glsl_version)
#endif
{
    _gui = gui;
#ifdef _WIN32
    _hwnd = hwnd;
    _hDC = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(_hDC, &pfd);
    SetPixelFormat(_hDC, pf, &pfd);
    _hRC = wglCreateContext(_hDC);
    wglMakeCurrent(_hDC, _hRC);
#else
    _window = wind;
    glfwSetErrorCallback(WindowManager::GLFWErrorCallback);
    glfwMakeContextCurrent(wind);
    glfwSwapInterval(1);
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    applyStyle();

#ifdef _WIN32
    ImGui_ImplWin32_InitForOpenGL(hwnd);
#else
    ImGui_ImplGlfw_InitForOpenGL(wind, true);
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    if (_gui)
        _gui->init();
}

void WindowManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
#ifdef _WIN32
    ImGui_ImplWin32_Shutdown();
    wglMakeCurrent(NULL, NULL);
    if (_hRC) {
        wglDeleteContext(_hRC);
        _hRC = NULL;
    }
    ReleaseDC(_hwnd, _hDC);
#else
    ImGui_ImplGlfw_Shutdown();
#endif
    ImGui::DestroyContext();
}

void WindowManager::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
#ifdef _WIN32
    ImGui_ImplWin32_NewFrame();
#else
    ImGui_ImplGlfw_NewFrame();
#endif
    ImGui::NewFrame();
}

void WindowManager::render() {
    if (_loadDialog && _loadDialog->ready()) {
        auto paths = _loadDialog->result();
        _loadDialog.reset();
        if (!paths.empty()) {
            std::ifstream in{paths[0]};
            if (in.is_open()) {
                nlohmann::json j;
                in >> j;
                auto loaded = j.get<std::vector<Palette>>();
                if (_gui)
                    _gui->_palettes = std::move(loaded);
                _palettePath = paths[0];
            }
        }
    }
    if (_modelLoadDialog && _modelLoadDialog->ready()) {
        auto paths = _modelLoadDialog->result();
        _modelLoadDialog.reset();
        if (!paths.empty()) {
            loadModel(paths[0]);
        }
    }
    if (_paletteSaveDialog && _paletteSaveDialog->ready()) {
        auto path = _paletteSaveDialog->result();
        _paletteSaveDialog.reset();
        if (!path.empty()) {
            nlohmann::json j = _gui->_palettes;
            std::ofstream out{path};
            if (out.is_open()) {
                out << j.dump(4);
                _palettePath = path;
                _lastSavePath = path;
                _savePopup = true;
            }
        }
    }
    if (_modelSaveDialog && _modelSaveDialog->ready()) {
        auto path = _modelSaveDialog->result();
        _modelSaveDialog.reset();
        if (!path.empty()) {
            saveModel(path);
            _lastSavePath = path;
            _savePopup = true;
        }
    }
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save to JSON")) {
                _paletteSaveDialog = std::make_unique<pfd::save_file>(
                    "Save Palette", _palettePath,
                    std::vector<std::string>{"JSON Files", "*.json"});
            }
            if (ImGui::MenuItem("Load from JSON")) {
                _loadDialog = std::make_unique<pfd::open_file>(
                    "Open Palette JSON", ".",
                    std::vector<std::string>{"JSON Files", "*.json"});
            }
            if (ImGui::MenuItem("Save Model")) {
                _modelSaveDialog = std::make_unique<pfd::save_file>(
                    "Save Model", "model.json",
                    std::vector<std::string>{"JSON Files", "*.json"});
            }
            if (ImGui::MenuItem("Load Model")) {
                _modelLoadDialog = std::make_unique<pfd::open_file>(
                    "Open Model", ".",
                    std::vector<std::string>{"JSON Files", "*.json"});
            }
            if (ImGui::MenuItem("Quit")) {
#ifdef _WIN32
                PostQuitMessage(0);
#else
                glfwSetWindowShouldClose(_window, GLFW_TRUE);
#endif
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (_savePopup) {
        ImGui::OpenPopup("Save Successful");
        _savePopup = false;
    }
    if (ImGui::BeginPopupModal("Save Successful", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Saved file to %s", _lastSavePath.c_str());
        if (ImGui::Button("OK"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Palette", nullptr, flags);
    ImGuiIO &io = ImGui::GetIO();
    if (ImGui::IsWindowHovered(
            ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseDelta.x);
        ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
    }

    if (_gui)
        _gui->drawTabs();

    ImGui::End();

    ImGui::Render();
    int fb_w, fb_h;
#ifdef _WIN32
    RECT rect;
    GetClientRect(_hwnd, &rect);
    fb_w = rect.right - rect.left;
    fb_h = rect.bottom - rect.top;
    glViewport(0, 0, fb_w, fb_h);
#else
    glfwGetFramebufferSize(_window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);
#endif
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
#ifdef _WIN32
        HDC backup_dc = wglGetCurrentDC();
        HGLRC backup_rc = wglGetCurrentContext();
#else
        GLFWwindow *backup_context = glfwGetCurrentContext();
#endif
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
#ifdef _WIN32
        wglMakeCurrent(backup_dc, backup_rc);
#else
        glfwMakeContextCurrent(backup_context);
#endif
    }

#ifdef _WIN32
    SwapBuffers(_hDC);
#else
    glfwSwapBuffers(_window);
#endif
}

void WindowManager::applyStyle() {
    ImGui::StyleColorsDark();
    ImGuiIO &io = ImGui::GetIO();

    std::filesystem::path fontsDir{"assets/fonts"};
    if (!std::filesystem::exists(fontsDir)) {
        fontsDir = std::filesystem::path{"../assets/fonts"};
    }

    ImFont *inter = nullptr;
    auto interPath = fontsDir / "Inter-Regular.ttf";
    if (std::filesystem::exists(interPath)) {
        inter = io.Fonts->AddFontFromFileTTF(interPath.string().c_str(), 16.0f);
    }

    auto hackPath = fontsDir / "Hack-Regular.ttf";
    if (std::filesystem::exists(hackPath)) {
        io.Fonts->AddFontFromFileTTF(hackPath.string().c_str(), 16.0f);
    }

    if (inter != nullptr) {
        io.FontDefault = inter;
    }

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;

    style.Colors[ImGuiCol_Text] = ImVec4(0.97f, 0.97f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.13f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = style.Colors[ImGuiCol_WindowBg];

    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.46f, 0.44f, 0.37f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.96f, 0.15f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.89f, 0.18f, 1.0f);

    style.Colors[ImGuiCol_Header] = ImVec4(0.29f, 0.29f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.46f, 0.44f, 0.37f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.96f, 0.15f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_HeaderHovered];
    style.Colors[ImGuiCol_ButtonActive] = style.Colors[ImGuiCol_HeaderActive];

    style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.85f, 0.94f, 1.0f);
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.40f, 0.85f, 0.94f, 1.0f);
}

#ifndef _WIN32
void WindowManager::GLFWErrorCallback(int error, const char *desc) {
    std::print(stderr, "[ERROR]: GLFW Error {}: {}", error, desc);
}
#endif

void WindowManager::saveModel(const std::filesystem::path &path) {
    if (!_gui)
        return;
    std::vector<Palette> good;
    for (const auto &p : _gui->_palettes) {
        if (p._good)
            good.push_back(p);
    }
    _gui->_generator->model().train(good);
    nlohmann::json j = _gui->_generator->model();
    std::ofstream out{path};
    if (out.is_open())
        out << j.dump(4);
}

void WindowManager::loadModel(const std::filesystem::path &path) {
    if (!_gui)
        return;
    std::ifstream in{path};
    if (!in.is_open())
        return;
    nlohmann::json j;
    in >> j;
    Model m = j.get<Model>();
    _gui->_generator->model() = std::move(m);
}
} // namespace uc
