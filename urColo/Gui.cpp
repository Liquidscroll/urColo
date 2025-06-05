#include "Gui.h"

#include "imgui/imgui.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <print>

namespace uc {
void GuiManager::init(GLFWwindow *wind, const char *glsl_version) {
    this->_palettes.emplace_back("default");
    this->_palettes.at(0).addSwatch("black", {0.0f, 0.0f, 0.0f, 1.0f});

    glfwSetErrorCallback(GuiManager::GLFWErrorCallback);
    _window = wind;

    glfwMakeContextCurrent(wind);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(wind, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
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

void GuiManager::drawPalettes() {
    int cols = (int)this->_palettes.size();

    if (ImGui::BeginTable("PaletteTable", cols,
                          ImGuiTableFlags_SizingFixedFit)) {
        std::size_t idx = 0;
        for (auto &p : this->_palettes) {
            ImGui::TableNextColumn();

            ImGui::PushID((int)idx);
            ImGui::Text("%s", p._name.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("+")) {
                _palettes.emplace_back();
            }
            ImGui::PopID();

            this->drawPalette(p, (int)idx);
            ++idx;
        }
        ImGui::EndTable();
    }
}

void GuiManager::drawPalette(uc::Palette &pal, int pal_idx) {

    constexpr float swatch_px = 64.0f;

    for (std::size_t i = 0; i < pal._swatches.size(); ++i) {
        this->drawSwatch(pal._swatches[i], pal_idx, (int)i, swatch_px);
    }

    ImGui::PushID(std::format("add_swatch-{}", pal_idx).c_str());
    if (ImGui::Button("+", ImVec2(swatch_px * 3, swatch_px))) {
        pal.addSwatch(
            std::format("pal-{}-swatch-{}", pal_idx, pal._swatches.size()),
            {0.0f, 0.0f, 0.0f, 1.0f});
    }
    ImGui::PopID();
}

void GuiManager::drawSwatch(uc::Swatch &sw, int pal_idx, int idx,
                            float swatch_px) {
    // constexpr int cols = 1;
    constexpr auto flags =
        ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker;

    ImGui::PushID(std::format("{}-{}", pal_idx, idx).c_str());

    ImGui::BeginGroup();
    if (ImGui::ColorButton("##swatch", sw._colour, flags,
                           ImVec2(swatch_px * 3, swatch_px))) {
        sw._locked = !sw._locked;
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImU32 border_col =
        sw._locked ? IM_COL32(255, 215, 0, 255) : IM_COL32(255, 255, 255, 255);
    float thickness = sw._locked ? 3.0f : 1.0f;
    dl->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), border_col,
                0.0f, 0, thickness);
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Checkbox("foreground", &sw._fg);
    ImGui::Checkbox("background", &sw._bg);
    ImGui::EndGroup();

    ImGui::PopID();
}

void GuiManager::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Palette", nullptr, flags);
    ImGui::Text("Palettes");
    drawPalettes();
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

void GuiManager::GLFWErrorCallback(int error, const char *desc) {
    std::print(stderr, "[ERROR]: GLFW Error {}: {}", error, desc);
}
} // namespace uc
