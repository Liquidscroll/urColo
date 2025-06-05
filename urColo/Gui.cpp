#include "Gui.h"

#include "imgui/imgui.h"

#include "Logger.h"
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

void GuiManager::startGeneration() {
    for (auto &p : _palettes) {
        std::vector<Swatch> locked;
        std::vector<std::size_t> unlocked_indices;

        for (std::size_t i = 0; i < p._swatches.size(); ++i) {
            if (p._swatches[i]._locked) {
                locked.push_back(p._swatches[i]);
            } else {
                unlocked_indices.push_back(i);
            }
        }

        if (unlocked_indices.empty()) {
            continue;
        }
        auto generated = _generator.generate(locked, unlocked_indices.size());

        for (std::size_t i = 0; i < unlocked_indices.size(); ++i) {
            auto idx = unlocked_indices[i];
            p._swatches[idx]._colour = generated[i]._colour;
            p._swatches[idx]._locked = false;
        }
    }
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
    applyPendingMoves();
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
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("UC_SWATCH")) {
            auto data = static_cast<const DragPayload *>(payload->Data);
            _pendingMoves.push_back(
                {data->pal_idx, data->swatch_idx, pal_idx,
                 static_cast<int>(pal._swatches.size())});
        }
        ImGui::EndDragDropTarget();
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
    const std::string picker_id =
        std::format("picker-{}-{}", pal_idx, idx);
    if (ImGui::ColorButton("##swatch", sw._colour, flags,
                           ImVec2(swatch_px * 3, swatch_px))) {
        ImGui::OpenPopup(picker_id.c_str());
    }
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        DragPayload payload{pal_idx, idx};
        ImGui::SetDragDropPayload("UC_SWATCH", &payload, sizeof(payload));
        ImGui::Text("%s", sw._name.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload("UC_SWATCH")) {
            auto data = static_cast<const DragPayload *>(p->Data);
            _pendingMoves.push_back({data->pal_idx, data->swatch_idx, pal_idx, idx});
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::BeginPopup(picker_id.c_str())) {
        ImGui::ColorPicker4("##picker", &sw._colour.x,
                            ImGuiColorEditFlags_NoAlpha);
        ImGui::EndPopup();
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
    if (ImGui::SmallButton(sw._locked ? "unlock" : "lock")) {
        sw._locked = !sw._locked;
    }
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
    if (ImGui::Button("start generation")) {
        startGeneration();
    }
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

void GuiManager::applyPendingMoves() {
    for (const auto &m : _pendingMoves) {
        if (m.from_pal < 0 || m.from_pal >= (int)_palettes.size() ||
            m.to_pal < 0 || m.to_pal >= (int)_palettes.size()) {
            continue;
        }

        auto &src = _palettes[m.from_pal];
        auto &dst = _palettes[m.to_pal];
        if (m.from_idx < 0 || m.from_idx >= (int)src._swatches.size()) {
            continue;
        }

        uc::Swatch sw = src._swatches[m.from_idx];
        src._swatches.erase(src._swatches.begin() + m.from_idx);

        int insert_idx = m.to_idx;
        if (m.from_pal == m.to_pal && m.from_idx < m.to_idx)
            insert_idx -= 1;

        insert_idx = std::clamp(insert_idx, 0, (int)dst._swatches.size());
        dst._swatches.insert(dst._swatches.begin() + insert_idx, sw);
    }
    _pendingMoves.clear();
}
} // namespace uc
