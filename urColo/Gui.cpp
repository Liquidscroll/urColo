#include "Gui.h"

#include "Gui/GenSettingsTab.h"
#include "Gui/HighlightsTab.h"
#include "Gui/PaletteGenTab.h"
#include "PaletteGenerator.h"
#include "imgui/imgui.h"

#include "Contrast.h"
#include "ImageUtils.h"
#include "Logger.h"
#include "Model.h"
#include "Profiling.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <GL/gl.h>

#include "imgui/misc/cpp/imgui_stdlib.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wshadow"
#include <portable-file-dialogs.h>
#pragma GCC diagnostic pop
#include <print>
#include <sstream>
#include <unordered_map>

namespace uc {

void GuiManager::init(GLFWwindow *wind, const char *glsl_version) {
    glfwSetErrorCallback(GuiManager::GLFWErrorCallback);
    _window = wind;

    glfwMakeContextCurrent(wind);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(wind, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    this->_palettes.emplace_back("default");
    this->_palettes.at(0).addSwatch("p0-#000000", {0.0f, 0.0f, 0.0f, 1.0f});

    _generator = new PaletteGenerator();

    _genSettingsTab = new GenSettingsTab(this, _generator);
    _paletteGenTab = new PaletteGenTab(this, _generator, _genSettingsTab);
    _hlTab = new HighlightsTab(this);
    _contrastTab = new ContrastTestTab(this);
}

void GuiManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GuiManager::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

const Swatch *GuiManager::swatchForIndex(int idx) const {
    if (idx < 0)
        return nullptr;
    for (const auto &p : _palettes) {
        if (idx < static_cast<int>(p._swatches.size()))
            return &p._swatches[(std::size_t)idx];
        idx -= static_cast<int>(p._swatches.size());
    }
    return nullptr;
}

void GuiManager::render() {
    if (_loadDialog && _loadDialog->ready()) {
        auto paths = _loadDialog->result();
        _loadDialog.reset();
        if (!paths.empty()) {
            std::ifstream in{paths[0]};
            if (in.is_open()) {
                nlohmann::json j;
                in >> j;
                auto loaded = j.get<std::vector<Palette>>();
                _palettes = std::move(loaded);
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
            nlohmann::json j = _palettes;
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
                glfwSetWindowShouldClose(_window, GLFW_TRUE);
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
    if (ImGui::BeginTabBar("main_tabs")) {
        if (ImGui::BeginTabItem("Palette Generation")) {
            _paletteGenTab->drawContent();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Generation Settings")) {
            _genSettingsTab->drawContent();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Contrast Testing")) {
            _contrastTab->drawContent();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Highlights")) {
            _hlTab->drawContent();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::Render();
    int fb_w, fb_h;
    glfwGetFramebufferSize(_window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);
    // glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(_window);
}

void GuiManager::applyStyle() {
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

    style.Colors[ImGuiCol_Text] = ImVec4(0.97f, 0.97f, 0.95f, 1.0f); // #F8F8F2
    style.Colors[ImGuiCol_WindowBg] =
        ImVec4(0.15f, 0.15f, 0.13f, 1.0f); // #272822
    style.Colors[ImGuiCol_PopupBg] = style.Colors[ImGuiCol_WindowBg];

    style.Colors[ImGuiCol_FrameBg] =
        ImVec4(0.24f, 0.24f, 0.20f, 1.0f); // #3E3D32
    style.Colors[ImGuiCol_FrameBgHovered] =
        ImVec4(0.46f, 0.44f, 0.37f, 1.0f); // #75715E
    style.Colors[ImGuiCol_FrameBgActive] =
        ImVec4(0.96f, 0.15f, 0.45f, 1.0f); // #F92672
    style.Colors[ImGuiCol_CheckMark] =
        ImVec4(0.65f, 0.89f, 0.18f, 1.0f); // #A6E22E

    style.Colors[ImGuiCol_Header] =
        ImVec4(0.29f, 0.29f, 0.25f, 1.0f); // #49483E
    style.Colors[ImGuiCol_HeaderHovered] =
        ImVec4(0.46f, 0.44f, 0.37f, 1.0f); // #75715E
    style.Colors[ImGuiCol_HeaderActive] =
        ImVec4(0.96f, 0.15f, 0.45f, 1.0f); // #F92672
    style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_HeaderHovered];
    style.Colors[ImGuiCol_ButtonActive] = style.Colors[ImGuiCol_HeaderActive];

    style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_TabHovered] =
        ImVec4(0.40f, 0.85f, 0.94f, 1.0f); // #66D9EF
    style.Colors[ImGuiCol_TabSelected] =
        ImVec4(0.40f, 0.85f, 0.94f, 1.0f); // #66D9EF
}

void GuiManager::GLFWErrorCallback(int error, const char *desc) {
    std::print(stderr, "[ERROR]: GLFW Error {}: {}", error, desc);
}

void GuiManager::saveModel(const std::filesystem::path &path) {
    std::vector<Palette> good;
    for (const auto &p : _palettes) {
        if (p._good)
            good.push_back(p);
    }
    _generator->model().train(good);
    nlohmann::json j = _generator->model();
    std::ofstream out{path};
    if (out.is_open())
        out << j.dump(4);
}

void GuiManager::loadModel(const std::filesystem::path &path) {
    std::ifstream in{path};
    if (!in.is_open())
        return;
    nlohmann::json j;
    in >> j;
    Model m = j.get<Model>();
    _generator->model() = std::move(m);
}
} // namespace uc
