#include "Gui.h"

#include "imgui/imgui.h"

#include "Logger.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
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
    this->_palettes.emplace_back("default");
    this->_palettes.at(0).addSwatch("black", {0.0f, 0.0f, 0.0f, 1.0f});

    _highlightGroups = {
        {"Comment", "Comments"},
        {"Constant", "any constant"},
        {"String", "\"this is a string\""},
        {"Character", "'c', '\\n'"},
        {"Number", "234, 0xff"},
        {"Boolean", "TRUE, false"},
        {"Float", "2.3e10"},
        {"Identifier", "variable"},
        {"Function", "function()"},
        {"Statement", "return"},
        {"Conditional", "if (x)"},
        {"Repeat", "for"},
        {"Label", "case"},
        {"Operator", "+, *"},
        {"Keyword", "static"},
        {"Exception", "try/catch"},
        {"PreProc", "#define"},
        {"Include", "#include"},
        {"Define", "#define"},
        {"Macro", "MACRO"},
        {"PreCondit", "#if"},
        {"Type", "MyType"},
        {"StorageClass", "static"},
        {"Structure", "struct"},
        {"Typedef", "typedef"},
        {"Special", "special"},
        {"SpecialChar", "\n"},
        {"Delimiter", ";"},
        {"SpecialComment", "TODO"},
        {"Underlined", "underlined"},
        {"Bold", "bold"},
        {"Italic", "italic"},
        {"Error", "error"},
        {"Todo", "TODO"},
        {"javaAnnotation", "@Override"},
    };

    for (std::size_t i = 0; i < _highlightGroups.size(); ++i) {
        _hlIndex[_highlightGroups[i].name] = static_cast<int>(i);
    }

    static const char *sample = R"(
#include <stdio.h>
#define PI 3.14

struct MyType {
    const char* str = "hello";
    int num = 42;
};

int main() {
    // TODO: implement
    for (int i = 0; i < num; ++i) {
        if (num > 0 && true) {
            printf("%c\n", 'A');
        } else {
            return 0;
        }
    }
}
    )";

    parseCodeSnippet(sample);

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
            _pendingMoves.push_back({data->pal_idx, data->swatch_idx, pal_idx,
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
    const std::string picker_id = std::format("picker-{}-{}", pal_idx, idx);
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
            _pendingMoves.push_back(
                {data->pal_idx, data->swatch_idx, pal_idx, idx});
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

void GuiManager::drawHighlights() {
    if (_palettes.empty()) {
        ImGui::Text("No palettes available");
        return;
    }

    auto &pal = _palettes[0];

    ImGui::Text("Global Defaults");
    std::string globalFgLabel =
        _globalFgSwatch >= 0 && _globalFgSwatch < (int)pal._swatches.size()
            ? pal._swatches[_globalFgSwatch]._name
            : "None";
    if (ImGui::BeginCombo("Default FG", globalFgLabel.c_str())) {
        if (ImGui::Selectable("None", _globalFgSwatch == -1))
            _globalFgSwatch = -1;
        for (int s = 0; s < (int)pal._swatches.size(); ++s) {
            bool selected = _globalFgSwatch == s;
            if (ImGui::Selectable(pal._swatches[s]._name.c_str(), selected))
                _globalFgSwatch = s;
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    std::string globalBgLabel =
        _globalBgSwatch >= 0 && _globalBgSwatch < (int)pal._swatches.size()
            ? pal._swatches[_globalBgSwatch]._name
            : "None";
    if (ImGui::BeginCombo("Default BG", globalBgLabel.c_str())) {
        if (ImGui::Selectable("None", _globalBgSwatch == -1))
            _globalBgSwatch = -1;
        for (int s = 0; s < (int)pal._swatches.size(); ++s) {
            bool selected = _globalBgSwatch == s;
            if (ImGui::Selectable(pal._swatches[s]._name.c_str(), selected))
                _globalBgSwatch = s;
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginTable("HLTable", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Group");
        ImGui::TableSetupColumn("Example");
        ImGui::TableSetupColumn("FG");
        ImGui::TableSetupColumn("BG");
        ImGui::TableHeadersRow();

        for (std::size_t i = 0; i < _highlightGroups.size(); ++i) {
            auto &hg = _highlightGroups[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(hg.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImVec4 fg = ImVec4(1, 1, 1, 1);
            ImVec4 bg = ImVec4(0, 0, 0, 1);
            if (hg.fgSwatch >= 0 && hg.fgSwatch < (int)pal._swatches.size())
                fg = pal._swatches[hg.fgSwatch]._colour;
            if (hg.bgSwatch >= 0 && hg.bgSwatch < (int)pal._swatches.size())
                bg = pal._swatches[hg.bgSwatch]._colour;

            ImVec2 min = ImGui::GetCursorScreenPos();
            ImVec2 textSize = ImGui::CalcTextSize(hg.sample.c_str());
            ImVec2 max = {min.x + textSize.x, min.y + textSize.y};
            ImGui::GetWindowDrawList()->AddRectFilled(
                min, max, ImGui::ColorConvertFloat4ToU32(bg));
            ImGui::PushStyleColor(ImGuiCol_Text, fg);
            ImGui::TextUnformatted(hg.sample.c_str());
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(2);
            std::string fgLabel =
                hg.fgSwatch >= 0 && hg.fgSwatch < (int)pal._swatches.size()
                    ? pal._swatches[hg.fgSwatch]._name
                    : "None";
            if (ImGui::BeginCombo(std::format("fg##{}", i).c_str(),
                                  fgLabel.c_str())) {
                if (ImGui::Selectable("None", hg.fgSwatch == -1))
                    hg.fgSwatch = -1;
                for (int s = 0; s < (int)pal._swatches.size(); ++s) {
                    bool selected = hg.fgSwatch == s;
                    if (ImGui::Selectable(pal._swatches[s]._name.c_str(),
                                          selected))
                        hg.fgSwatch = s;
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::TableSetColumnIndex(3);
            std::string bgLabel =
                hg.bgSwatch >= 0 && hg.bgSwatch < (int)pal._swatches.size()
                    ? pal._swatches[hg.bgSwatch]._name
                    : "None";
            if (ImGui::BeginCombo(std::format("bg##{}", i).c_str(),
                                  bgLabel.c_str())) {
                if (ImGui::Selectable("None", hg.bgSwatch == -1))
                    hg.bgSwatch = -1;
                for (int s = 0; s < (int)pal._swatches.size(); ++s) {
                    bool selected = hg.bgSwatch == s;
                    if (ImGui::Selectable(pal._swatches[s]._name.c_str(),
                                          selected))
                        hg.bgSwatch = s;
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    drawCodePreview();
}

void GuiManager::drawCodePreview() {
    if (_palettes.empty())
        return;
    auto &pal = _palettes[0];

    for (const auto &line : _codeSample) {
        for (std::size_t i = 0; i < line.size(); ++i) {
            const auto &tok = line[i];
            ImVec4 fg = ImVec4(1, 1, 1, 1);
            ImVec4 bg = ImVec4(0, 0, 0, 0);
            if (_globalFgSwatch >= 0 &&
                _globalFgSwatch < (int)pal._swatches.size())
                fg = pal._swatches[_globalFgSwatch]._colour;
            if (_globalBgSwatch >= 0 &&
                _globalBgSwatch < (int)pal._swatches.size())
                bg = pal._swatches[_globalBgSwatch]._colour;
            if (tok.groupIdx >= 0 &&
                tok.groupIdx < (int)_highlightGroups.size()) {
                const auto &hg = _highlightGroups[tok.groupIdx];
                if (hg.fgSwatch >= 0 && hg.fgSwatch < (int)pal._swatches.size())
                    fg = pal._swatches[hg.fgSwatch]._colour;
                if (hg.bgSwatch >= 0 && hg.bgSwatch < (int)pal._swatches.size())
                    bg = pal._swatches[hg.bgSwatch]._colour;
            }

            ImVec2 min = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::CalcTextSize(tok.text.c_str());
            ImVec2 max = {min.x + size.x, min.y + size.y};
            if (bg.w > 0.0f)
                ImGui::GetWindowDrawList()->AddRectFilled(
                    min, max, ImGui::ColorConvertFloat4ToU32(bg));
            ImGui::PushStyleColor(ImGuiCol_Text, fg);
            ImGui::TextUnformatted(tok.text.c_str());
            ImGui::PopStyleColor();
            if (i + 1 < line.size())
                ImGui::SameLine(0.0f, 0.0f);
        }
    }
}

void GuiManager::parseCodeSnippet(const std::string &code) {
    _codeSample.clear();

    auto id = [&](const std::string &n) {
        auto it = _hlIndex.find(n);
        return it != _hlIndex.end() ? it->second : -1;
    };

    const std::unordered_map<std::string, std::string> tokenMap = {
        {"#include", "Include"},   {"#define", "Define"},
        {"PI", "Macro"},           {"struct", "Structure"},
        {"const", "StorageClass"}, {"char", "Type"},
        {"int", "Type"},           {"float", "Type"},
        {"return", "Statement"},   {"for", "Repeat"},
        {"if", "Conditional"},     {"else", "Conditional"},
        {"true", "Boolean"},       {"false", "Boolean"},
        {"printf", "Function"},    {"main", "Function"},
        {"MyType", "Type"},
    };

    std::istringstream iss(code);
    std::string line;
    while (std::getline(iss, line)) {
        CodeLine parsed;
        std::size_t i = 0;
        while (i < line.size()) {
            char c = line[i];
            if (std::isspace(static_cast<unsigned char>(c))) {
                std::string ws;
                while (i < line.size() &&
                       std::isspace(static_cast<unsigned char>(line[i]))) {
                    ws += line[i++];
                }
                parsed.push_back({ws, -1});
                continue;
            }

            if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                std::string comment = line.substr(i);
                std::size_t todoPos = comment.find("TODO");
                int commentIdx = id("Comment");
                int todoIdx = id("Todo");
                if (todoPos != std::string::npos) {
                    if (todoPos > 0)
                        parsed.push_back(
                            {comment.substr(0, todoPos), commentIdx});
                    parsed.push_back({"TODO", todoIdx});
                    parsed.push_back({comment.substr(todoPos + 4), commentIdx});
                } else {
                    parsed.push_back({comment, commentIdx});
                }
                break;
            }

            if (c == '"') {
                std::string str;
                str += c;
                ++i;
                while (i < line.size()) {
                    str += line[i];
                    if (line[i] == '\\' && i + 1 < line.size()) {
                        ++i;
                        str += line[i];
                    } else if (line[i] == '"') {
                        ++i;
                        break;
                    }
                    ++i;
                }
                parsed.push_back({str, id("String")});
                continue;
            }

            if (c == '\'') {
                std::string chr;
                chr += c;
                ++i;
                while (i < line.size()) {
                    chr += line[i];
                    if (line[i] == '\\' && i + 1 < line.size()) {
                        ++i;
                        chr += line[i];
                    } else if (line[i] == '\'') {
                        ++i;
                        break;
                    }
                    ++i;
                }
                parsed.push_back({chr, id("Character")});
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(c))) {
                std::string num;
                while (i < line.size() &&
                       (std::isdigit(static_cast<unsigned char>(line[i])) ||
                        line[i] == '.' || line[i] == 'e' || line[i] == 'E')) {
                    num += line[i++];
                }
                if (num.find('.') != std::string::npos)
                    parsed.push_back({num, id("Float")});
                else
                    parsed.push_back({num, id("Number")});
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' ||
                c == '#') {
                std::string word;
                while (i < line.size() &&
                       (std::isalnum(static_cast<unsigned char>(line[i])) ||
                        line[i] == '_' || line[i] == '#')) {
                    word += line[i++];
                }
                int idx = id("Identifier");
                auto tm = tokenMap.find(word);
                if (tm != tokenMap.end())
                    idx = id(tm->second);
                parsed.push_back({word, idx});
                continue;
            }

            std::string sym(1, c);
            if (i + 1 < line.size()) {
                char next = line[i + 1];
                if ((c == '+' && next == '+') || (c == '-' && next == '-') ||
                    next == '=' || (c == '&' && next == '&') ||
                    (c == '|' && next == '|')) {
                    sym += next;
                    ++i;
                }
            }
            ++i;

            int idx = id("Operator");
            if (std::string("(){}[];,<>").find(sym[0]) != std::string::npos)
                idx = id("Delimiter");
            parsed.push_back({sym, idx});
        }
        _codeSample.push_back(std::move(parsed));
    }
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
            }
        }
    }
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save to JSON")) {
                nlohmann::json j = _palettes;
                _lastSavePath = "palettes.json";
                std::ofstream out{_lastSavePath};
                if (out.is_open()) {
                    out << j.dump(4);
                    _savePopup = true;
                }
            }
            if (ImGui::MenuItem("Load from JSON")) {
                _loadDialog = std::make_unique<pfd::open_file>(
                    "Open Palette JSON", ".",
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
        ImGui::Text("Saved palettes to %s", _lastSavePath.c_str());
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
    if (ImGui::BeginTabBar("main_tabs")) {
        if (ImGui::BeginTabItem("Palettes")) {
            ImGui::Text("Palettes");
            if (ImGui::Button("start generation")) {
                startGeneration();
            }
            drawPalettes();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Highlights")) {
            drawHighlights();
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
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.FrameBorderSize = 1.0f;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.115f, 0.124f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.225f, 0.23f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.325f, 0.33f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.225f, 0.23f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.22f, 0.225f, 0.23f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.325f, 0.33f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.175f, 0.18f, 1.0f);
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
