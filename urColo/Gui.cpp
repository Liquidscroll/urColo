#include "Gui.h"

#include "imgui/imgui.h"

#include "Contrast.h"
#include "ImageUtils.h"
#include "Logger.h"
#include "Model.h"
#include "Profiling.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

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
    this->_palettes.emplace_back("default");
    this->_palettes.at(0).addSwatch("p0-#000000", {0.0f, 0.0f, 0.0f, 1.0f});

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
    if (_genRunning)
        return;

    _generator.clearKMeansImage();

    auto generator = _generator; // copy for thread safety and RNG advance
    auto palettes = _palettes;
    auto mode = _genMode;
    auto imgSource = _imageSource;
    auto imgCols = _imageColours;
    int rW = _randWidth;
    int rH = _randHeight;

    auto work = [generator, palettes, mode, imgSource, imgCols, rW,
                 rH]() mutable {
        if (generator.algorithm() == PaletteGenerator::Algorithm::KMeans) {
            if (imgSource == ImageSource::Loaded && !imgCols.empty()) {
                generator.setKMeansImage(imgCols);
            } else if (imgSource == ImageSource::Random) {
                generator.setKMeansRandomImage(rW, rH);
            } else {
                generator.clearKMeansImage();
            }
        }

        if (mode == GenerationMode::PerPalette) {
            for (auto &p : palettes) {
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
                auto generated =
                    generator.generate(locked, unlocked_indices.size());

                for (std::size_t i = 0; i < unlocked_indices.size(); ++i) {
                    auto idx = unlocked_indices[i];
                    p._swatches[idx]._colour = generated[i]._colour;
                    p._swatches[idx]._locked = false;
                }
            }
        } else {
            std::vector<Swatch> locked;
            std::vector<Swatch *> unlocked;

            for (auto &p : palettes) {
                for (auto &sw : p._swatches) {
                    if (sw._locked) {
                        locked.push_back(sw);
                    } else {
                        unlocked.push_back(&sw);
                    }
                }
            }

            if (!unlocked.empty()) {
                auto generated = generator.generate(locked, unlocked.size());
                for (std::size_t i = 0; i < unlocked.size(); ++i) {
                    unlocked[i]->_colour = generated[i]._colour;
                    unlocked[i]->_locked = false;
                }
            }
        }

        return std::make_pair(palettes, generator);
    };

    bool slow = _generator.algorithm() == PaletteGenerator::Algorithm::KMeans;
    if (slow) {
        _genRunning = true;
        _genReady = false;
        _genThread = std::jthread([this, work]() mutable {
            auto [result, gen] = work();
            {
                std::lock_guard<std::mutex> lock(_genMutex);
                _genResult = std::move(result);
            }
            _generator = gen;
            _genReady = true;
            _genRunning = false;
        });
    } else {
        auto [result, gen] = work();
        _palettes = std::move(result);
        _generator = gen;
    }
}

void GuiManager::drawPalettes(bool showFgBg, bool dragAndLock) {
    int cols = (int)this->_palettes.size();

    if (ImGui::BeginTable("PaletteTable", cols,
                          ImGuiTableFlags_SizingFixedFit |
                              ImGuiTableFlags_ScrollX)) {
        std::size_t idx = 0;
        for (auto &p : this->_palettes) {
            ImGui::TableNextColumn();

            ImGui::PushID((int)idx);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
            ImGui::Button(p._name.c_str(), ImVec2(-FLT_MIN, 0));
            ImGui::PopStyleColor();

            if (dragAndLock) {
                if (ImGui::BeginDragDropSource(
                        ImGuiDragDropFlags_SourceAllowNullID)) {
                    int payload = static_cast<int>(idx);
                    ImGui::SetDragDropPayload("UC_PALETTE", &payload,
                                              sizeof(payload));
                    ImGui::TextUnformatted(p._name.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload *pl =
                            ImGui::AcceptDragDropPayload("UC_PALETTE")) {
                        int src = *static_cast<const int *>(pl->Data);
                        _pendingPaletteMoves.push_back(
                            {src, static_cast<int>(idx)});
                    }
                    ImGui::EndDragDropTarget();
                }
            }

            ImGui::SetNextItemWidth(g_swatchWidthPx);
            ImGui::InputText("##pal_name", &p._name);

            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(0, 25))) {
                _palettes.emplace_back(
                    std::format("palette {}", _palettes.size() + 1));
            }
            if (_palettes.size() > 1) {
                ImGui::SameLine();
                if (ImGui::Button("X", ImVec2(0, 25))) {
                    _pendingPaletteDeletes.push_back(static_cast<int>(idx));
                }
            }
            ImGui::PopID();

            this->drawPalette(p, (int)idx, showFgBg, dragAndLock);
            ++idx;
        }
        ImGui::EndTable();
    }
    applyPendingMoves();
}

void GuiManager::drawPalette(uc::Palette &pal, int pal_idx, bool showFgBg,
                             bool dragAndLock) {

    const float swatch_width_px = g_swatchWidthPx;
    const float swatch_height_px = g_swatchHeightPx;

    ImGui::PushID(std::format("pal-controls-{}", pal_idx).c_str());
    bool anyButtons = false;
    if (showFgBg) {
        anyButtons = true;
        if (ImGui::SmallButton("All FG")) {
            for (auto &sw : pal._swatches)
                sw._fg = !sw._fg;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("All BG")) {
            for (auto &sw : pal._swatches)
                sw._bg = !sw._bg;
        }
        if (dragAndLock)
            ImGui::SameLine();
    }
    if (dragAndLock) {
        anyButtons = true;
        if (ImGui::SmallButton("Lock all")) {
            for (auto &sw : pal._swatches)
                sw._locked = !sw._locked;
        }
        ImGui::SameLine();
        ImGui::Checkbox("good", &pal._good);
    }
    if (anyButtons)
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * 0.5f));
    ImGui::PopID();

    for (std::size_t i = 0; i < pal._swatches.size(); ++i) {
        this->drawSwatch(pal._swatches[i], pal_idx, (int)i, swatch_width_px,
                         swatch_height_px, showFgBg, dragAndLock);
    }

    ImGui::PushID(std::format("add_swatch-{}", pal_idx).c_str());
    if (ImGui::Button("+", ImVec2(swatch_width_px * 3, swatch_height_px))) {
        ImVec4 col{0.0f, 0.0f, 0.0f, 1.0f};
        std::string hex = toHexString(col);
        pal.addSwatch(std::format("p{}-{}", pal_idx, hex), col);
    }
    if (dragAndLock) {
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
    }
    ImGui::PopID();
}

void GuiManager::drawSwatch(uc::Swatch &sw, int pal_idx, int idx,
                            float swatch_width_px, float swatch_height_px,
                            bool showFgBg, bool dragAndLock) {
    // constexpr int cols = 1;
    constexpr auto flags =
        ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker;

    ImGui::PushID(std::format("{}-{}", pal_idx, idx).c_str());

    ImGui::BeginGroup();
    const std::string picker_id = std::format("picker-{}-{}", pal_idx, idx);
    if (ImGui::ColorButton("##swatch", sw._colour, flags,
                           ImVec2(swatch_width_px * 3, swatch_height_px))) {
        ImGui::OpenPopup(picker_id.c_str());
    }
    if (dragAndLock) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            DragPayload payload{pal_idx, idx};
            ImGui::SetDragDropPayload("UC_SWATCH", &payload, sizeof(payload));
            ImGui::Text("%s", sw._name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *p =
                    ImGui::AcceptDragDropPayload("UC_SWATCH")) {
                auto data = static_cast<const DragPayload *>(p->Data);
                _pendingMoves.push_back(
                    {data->pal_idx, data->swatch_idx, pal_idx, idx});
            }
            ImGui::EndDragDropTarget();
        }
    }
    if (ImGui::BeginPopup(picker_id.c_str())) {
        if (ImGui::ColorPicker4("##picker", &sw._colour.x,
                                ImGuiColorEditFlags_NoAlpha)) {
            sw._hex = toHexString(sw._colour);
            sw._name = std::format("p{}-{}", pal_idx, sw._hex);
        }
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
    float hexWidth =
        ImGui::CalcTextSize("#FFFFFF").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetNextItemWidth(hexWidth);
    bool hexEdited = ImGui::InputText(
        std::format("##hex-{}-{}", pal_idx, idx).c_str(), &sw._hex);
    if (hexEdited) {
        ImVec4 col;
        if (hexToColour(sw._hex, col)) {
            sw._colour = col;
        }
        sw._name = std::format("p{}-{}", pal_idx, sw._hex);
    } else if (!ImGui::IsItemActive()) {
        std::string newHex = toHexString(sw._colour);
        if (newHex != sw._hex) {
            sw._hex = newHex;
            sw._name = std::format("p{}-{}", pal_idx, sw._hex);
        }
    }

    if (dragAndLock) {
        if (ImGui::Button(sw._locked ? "unlock" : "lock", ImVec2(0, 25))) {
            sw._locked = !sw._locked;
        }
        ImGui::SameLine();
    }
    if (showFgBg) {
        ImGui::Checkbox("fg", &sw._fg);
        ImGui::SameLine();
        ImGui::Checkbox("bg", &sw._bg);
    }
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
            ? pal._swatches[_globalFgSwatch]._hex
            : "None";
    if (ImGui::BeginCombo("Default FG", globalFgLabel.c_str())) {
        if (ImGui::Selectable("None", _globalFgSwatch == -1))
            _globalFgSwatch = -1;
        for (int s = 0; s < (int)pal._swatches.size(); ++s) {
            bool selected = _globalFgSwatch == s;
            if (ImGui::Selectable(pal._swatches[s]._hex.c_str(), selected))
                _globalFgSwatch = s;
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    std::string globalBgLabel =
        _globalBgSwatch >= 0 && _globalBgSwatch < (int)pal._swatches.size()
            ? pal._swatches[_globalBgSwatch]._hex
            : "None";
    if (ImGui::BeginCombo("Default BG", globalBgLabel.c_str())) {
        if (ImGui::Selectable("None", _globalBgSwatch == -1))
            _globalBgSwatch = -1;
        for (int s = 0; s < (int)pal._swatches.size(); ++s) {
            bool selected = _globalBgSwatch == s;
            if (ImGui::Selectable(pal._swatches[s]._hex.c_str(), selected))
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
            ImDrawList *dl = ImGui::GetWindowDrawList();
            dl->ChannelsSplit(3);
            dl->ChannelsSetCurrent(2);
            ImVec4 fg = ImVec4(1, 1, 1, 1);
            ImVec4 bg = ImVec4(0, 0, 0, 0);
            if (hg.fgSwatch >= 0 && hg.fgSwatch < (int)pal._swatches.size())
                fg = pal._swatches[hg.fgSwatch]._colour;
            if (hg.bgSwatch >= 0 && hg.bgSwatch < (int)pal._swatches.size())
                bg = pal._swatches[hg.bgSwatch]._colour;

            ImVec2 min = ImGui::GetCursorScreenPos();
            ImVec2 textSize = ImGui::CalcTextSize(hg.sample.c_str());
            ImVec2 max = {min.x + textSize.x, min.y + textSize.y};
            if (_globalBgSwatch >= 0 &&
                _globalBgSwatch < (int)pal._swatches.size()) {
                dl->ChannelsSetCurrent(0);
                ImVec4 gbg = pal._swatches[_globalBgSwatch]._colour;
                dl->AddRectFilled(min, max,
                                  ImGui::ColorConvertFloat4ToU32(gbg));
            }
            if (bg.w > 0.0f) {
                dl->ChannelsSetCurrent(1);
                dl->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(bg));
            }
            dl->ChannelsSetCurrent(2);
            ImGui::PushStyleColor(ImGuiCol_Text, fg);
            ImGui::TextUnformatted(hg.sample.c_str());
            ImGui::PopStyleColor();
            dl->ChannelsMerge();

            ImGui::TableSetColumnIndex(2);
            std::string fgLabel =
                hg.fgSwatch >= 0 && hg.fgSwatch < (int)pal._swatches.size()
                    ? pal._swatches[hg.fgSwatch]._hex
                    : "None";
            if (ImGui::BeginCombo(std::format("fg##{}", i).c_str(),
                                  fgLabel.c_str())) {
                if (ImGui::Selectable("None", hg.fgSwatch == -1))
                    hg.fgSwatch = -1;
                for (int s = 0; s < (int)pal._swatches.size(); ++s) {
                    bool selected = hg.fgSwatch == s;
                    if (ImGui::Selectable(pal._swatches[s]._hex.c_str(),
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
                    ? pal._swatches[hg.bgSwatch]._hex
                    : "None";
            if (ImGui::BeginCombo(std::format("bg##{}", i).c_str(),
                                  bgLabel.c_str())) {
                if (ImGui::Selectable("None", hg.bgSwatch == -1))
                    hg.bgSwatch = -1;
                for (int s = 0; s < (int)pal._swatches.size(); ++s) {
                    bool selected = hg.bgSwatch == s;
                    if (ImGui::Selectable(pal._swatches[s]._hex.c_str(),
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

    ImVec2 start = ImGui::GetCursorScreenPos();
    ImVec2 end = start;
    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->ChannelsSplit(3);
    dl->ChannelsSetCurrent(2);
    for (const auto &line : _codeSample) {
        for (std::size_t i = 0; i < line.size(); ++i) {
            const auto &tok = line[i];
            ImVec4 fg = ImVec4(1, 1, 1, 1);
            ImVec4 bg = ImVec4(0, 0, 0, 0);
            if (_globalFgSwatch >= 0 &&
                _globalFgSwatch < (int)pal._swatches.size())
                fg = pal._swatches[_globalFgSwatch]._colour;
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
            if (bg.w > 0.0f) {
                dl->ChannelsSetCurrent(1);
                dl->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(bg));
                dl->ChannelsSetCurrent(2);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, fg);
            ImGui::TextUnformatted(tok.text.c_str());
            ImGui::PopStyleColor();
            end.x = std::max(end.x, max.x);
            end.y = std::max(end.y, max.y);
            if (i + 1 < line.size())
                ImGui::SameLine(0.0f, 0.0f);
        }
    }
    if (_globalBgSwatch >= 0 && _globalBgSwatch < (int)pal._swatches.size()) {
        dl->ChannelsSetCurrent(0);
        ImVec4 bg = pal._swatches[_globalBgSwatch]._colour;
        dl->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(bg));
    }
    dl->ChannelsMerge();
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
    if (_genReady) {
        std::lock_guard<std::mutex> lock(_genMutex);
        if (!_genResult.empty()) {
            _palettes = std::move(_genResult);
            _genResult.clear();
        }
        _genReady = false;
    }
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
    if (_modelLoadDialog && _modelLoadDialog->ready()) {
        auto paths = _modelLoadDialog->result();
        _modelLoadDialog.reset();
        if (!paths.empty()) {
            loadModel(paths[0]);
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
    if (_imageDialog && _imageDialog->ready()) {
        auto paths = _imageDialog->result();
        _imageDialog.reset();
        if (!paths.empty()) {
            _imageColours = loadImageColours(paths[0]);
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
            if (ImGui::MenuItem("Load Image")) {
                _imageDialog = std::make_unique<pfd::open_file>(
                    "Open Image", ".",
                    std::vector<std::string>{"Image Files",
                                             "*.png *.jpg *.bmp"});
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
    if (_contrastPopup) {
        ImGui::OpenPopup("Contrast Report");
        _contrastPopup = false;
    }
    if (ImGui::BeginPopupModal("Contrast Report", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("ContrastTable", 6,
                              ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("FG");
            ImGui::TableSetupColumn("BG");
            ImGui::TableSetupColumn("Sample");
            ImGui::TableSetupColumn("Ratio");
            ImGui::TableSetupColumn("AA");
            ImGui::TableSetupColumn("AAA");
            ImGui::TableHeadersRow();
            for (const auto &r : _contrastResults) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(r.fgName.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(r.bgName.c_str());
                ImGui::TableSetColumnIndex(2);
                ImVec2 min = ImGui::GetCursorScreenPos();
                ImVec2 textSize = ImGui::CalcTextSize("TEXT");
                ImVec2 max = {min.x + textSize.x, min.y + textSize.y};
                ImGui::GetWindowDrawList()->AddRectFilled(
                    min, max, ImGui::ColorConvertFloat4ToU32(r.bgCol));
                ImGui::PushStyleColor(ImGuiCol_Text, r.fgCol);
                ImGui::TextUnformatted("TEXT");
                ImGui::PopStyleColor();
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2f", r.ratio);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(r.aa ? "pass" : "fail");
                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(r.aaa ? "pass" : "fail");
            }
            ImGui::EndTable();
        } else {
            ImGui::TextUnformatted("No contrast pairs found");
        }
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
            ImGui::BeginDisabled(_genRunning);
            if (ImGui::Button("start generation")) {
                startGeneration();
            }
            ImGui::EndDisabled();
            if (_genRunning) {
                ImGui::SameLine();
                ImGui::TextUnformatted("Generating...");
            }
            drawPalettes(false, true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Generation Settings")) {
            static const char *algNames[] = {"Random Offset", "K-Means++",
                                             "Gradient", "Learned"};
            auto alg = _generator.algorithm();

            const float margin = ImGui::GetStyle().FramePadding.x * 2.0f;
            const float arrow = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("Random Offset").x +
                                    margin + arrow);
            if (ImGui::BeginCombo("Algorithm", algNames[(int)alg])) {
                for (int i = 0; i < 4; ++i) {
                    bool sel = i == (int)alg;
                    if (ImGui::Selectable(algNames[i], sel))
                        _generator.setAlgorithm(
                            static_cast<PaletteGenerator::Algorithm>(i));
                    if (sel)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (alg == PaletteGenerator::Algorithm::KMeans) {
                int it = _generator.kMeansIterations();
                ImGui::SetNextItemWidth(ImGui::CalcTextSize("200").x * 5.0f);
                if (ImGui::DragInt("Iterations", &it, 1.0f, 1, 200))
                    _generator.setKMeansIterations(it);

                static const char *srcNames[] = {"None", "Image", "Random"};
                int src = static_cast<int>(_imageSource);
                ImGui::SetNextItemWidth(ImGui::CalcTextSize("Random").x +
                                        margin + arrow);
                if (ImGui::BeginCombo("KMeans Source", srcNames[src])) {
                    for (int i = 0; i < 3; ++i) {
                        bool sel = i == src;
                        if (ImGui::Selectable(srcNames[i], sel))
                            _imageSource = static_cast<ImageSource>(i);
                        if (sel)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (_imageSource == ImageSource::Random) {
                    const float field = ImGui::CalcTextSize("512").x * 5.0f;
                    ImGui::SetNextItemWidth(field);
                    ImGui::DragInt("Width", &_randWidth, 1.0f, 1, 512);
                    ImGui::SetNextItemWidth(field);
                    ImGui::DragInt("Height", &_randHeight, 1.0f, 1, 512);
                }
            }
            static const char *modeNames[] = {"Per Palette", "All Palettes"};
            int mode = static_cast<int>(_genMode);
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("All Palettes").x +
                                    margin + arrow);
            if (ImGui::BeginCombo("Generation Mode", modeNames[mode])) {
                for (int i = 0; i < 2; ++i) {
                    bool sel = i == mode;
                    if (ImGui::Selectable(modeNames[i], sel))
                        _genMode = static_cast<GenerationMode>(i);
                    if (sel)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Contrast Testing")) {
            if (ImGui::Button("Run Contrast Tests")) {
                runContrastTests();
            }
            drawPalettes(true, false);
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
    style.WindowRounding = 6.0f;
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;

    style.Colors[ImGuiCol_Text]          = ImVec4(0.97f, 0.97f, 0.95f, 1.0f); // #F8F8F2
    style.Colors[ImGuiCol_WindowBg]      = ImVec4(0.15f, 0.15f, 0.13f, 1.0f); // #272822
    style.Colors[ImGuiCol_PopupBg]       = style.Colors[ImGuiCol_WindowBg];

    style.Colors[ImGuiCol_FrameBg]       = ImVec4(0.24f, 0.24f, 0.20f, 1.0f); // #3E3D32
    style.Colors[ImGuiCol_FrameBgHovered]= ImVec4(0.46f, 0.44f, 0.37f, 1.0f); // #75715E
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.96f, 0.15f, 0.45f, 1.0f); // #F92672
    style.Colors[ImGuiCol_CheckMark]     = ImVec4(0.65f, 0.89f, 0.18f, 1.0f); // #A6E22E

    style.Colors[ImGuiCol_Header]        = ImVec4(0.29f, 0.29f, 0.25f, 1.0f); // #49483E
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.46f, 0.44f, 0.37f, 1.0f); // #75715E
    style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.96f, 0.15f, 0.45f, 1.0f); // #F92672
    style.Colors[ImGuiCol_Button]        = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_HeaderHovered];
    style.Colors[ImGuiCol_ButtonActive]  = style.Colors[ImGuiCol_HeaderActive];

    style.Colors[ImGuiCol_Tab]           = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_TabHovered]    = ImVec4(0.40f, 0.85f, 0.94f, 1.0f); // #66D9EF
    style.Colors[ImGuiCol_TabSelected]   = ImVec4(0.40f, 0.85f, 0.94f, 1.0f); // #66D9EF
}

void GuiManager::GLFWErrorCallback(int error, const char *desc) {
    std::print(stderr, "[ERROR]: GLFW Error {}: {}", error, desc);
}

void GuiManager::runContrastTests() {
    _contrastResults.clear();
    std::vector<const Swatch *> fgs;
    std::vector<const Swatch *> bgs;
    for (const auto &p : _palettes) {
        for (const auto &sw : p._swatches) {
            if (sw._fg)
                fgs.push_back(&sw);
            if (sw._bg)
                bgs.push_back(&sw);
        }
    }
    for (const Swatch *fg : fgs) {
        for (const Swatch *bg : bgs) {
            PROFILE_FROM_IMVEC4();
            Colour fgc = Colour::fromImVec4(fg->_colour);
            PROFILE_FROM_IMVEC4();
            Colour bgc = Colour::fromImVec4(bg->_colour);
            double ratio = ContrastChecker::ratio(fgc, bgc);
            bool aa = ContrastChecker::passesAA(fgc, bgc);
            bool aaa = ContrastChecker::passesAAA(fgc, bgc);
            _contrastResults.push_back(
                {fg->_hex, bg->_hex, fg->_colour, bg->_colour, ratio, aa, aaa});
        }
    }
    _contrastPopup = true;
}

void GuiManager::saveModel(const std::filesystem::path &path) {
    std::vector<Palette> good;
    for (const auto &p : _palettes) {
        if (p._good)
            good.push_back(p);
    }
    _generator.model().train(good);
    nlohmann::json j = _generator.model();
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
    _generator.model() = std::move(m);
}

void GuiManager::applyPendingMoves() {
    if (!_pendingPaletteDeletes.empty()) {
        std::sort(_pendingPaletteDeletes.rbegin(),
                  _pendingPaletteDeletes.rend());
        for (int idx : _pendingPaletteDeletes) {
            if (idx < 0 || idx >= (int)_palettes.size())
                continue;

            _palettes.erase(_palettes.begin() + idx);

            for (auto &m : _pendingMoves) {
                if (m.from_pal > idx)
                    --m.from_pal;
                if (m.to_pal > idx)
                    --m.to_pal;
            }
            for (auto &pm : _pendingPaletteMoves) {
                if (pm.from_idx > idx)
                    --pm.from_idx;
                if (pm.to_idx > idx)
                    --pm.to_idx;
            }
        }
        _pendingPaletteDeletes.clear();
    }

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

    for (const auto &pm : _pendingPaletteMoves) {
        if (pm.from_idx < 0 || pm.from_idx >= (int)_palettes.size() ||
            pm.to_idx < 0 || pm.to_idx >= (int)_palettes.size()) {
            continue;
        }

        uc::Palette pal = _palettes[pm.from_idx];
        _palettes.erase(_palettes.begin() + pm.from_idx);

        int insert_idx = pm.to_idx;
        insert_idx = std::clamp(insert_idx, 0, (int)_palettes.size());
        _palettes.insert(_palettes.begin() + insert_idx, pal);
    }
    _pendingPaletteMoves.clear();

    for (std::size_t pi = 0; pi < _palettes.size(); ++pi) {
        for (auto &sw : _palettes[pi]._swatches) {
            if (sw._hex.empty())
                sw._hex = toHexString(sw._colour);
            sw._name = std::format("p{}-{}", pi, sw._hex);
        }
    }
}
} // namespace uc
