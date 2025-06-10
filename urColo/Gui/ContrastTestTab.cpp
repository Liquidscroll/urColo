#include "ContrastTestTab.h"
#include "Contrast.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "../Gui.h"

#include <format>

namespace uc {
ContrastTestTab::ContrastTestTab(GuiManager *manager)
    : Tab("Highlights", manager) {}

void ContrastTestTab::drawContent() {

    if (ImGui::Button("Run Contrast Tests")) {
        runContrastTests();
    }

    if (_contrastPopup) {
        ImGui::OpenPopup("Contrast Report");
        _contrastPopup = false;
    }

    drawContrastTestUi();
    drawContrastPopup();
}

void ContrastTestTab::drawContrastPopup() {
    if (ImGui::BeginPopupModal("Contrast Report", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Contrast Table", 6,
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
            ImGui::TextUnformatted("No contrast pairs found.");
        }

        if (ImGui::Button("OK"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void ContrastTestTab::runContrastTests() {
    _contrastResults.clear();

    std::vector<const Swatch *> fgs;
    std::vector<const Swatch *> bgs;

    for (const auto &p : _manager->_palettes) {
        for (const auto &sw : p._swatches) {
            if (sw._fg)
                fgs.push_back(&sw);
            if (sw._bg)
                bgs.push_back(&sw);
        }
    }

    for (const Swatch *fg : fgs) {
        for (const Swatch *bg : bgs) {
            // PROFILE_FROM_IMVEC4();
            Colour fgc = Colour::fromImVec4(fg->_colour);
            // PROFILE_FROM_IMVEC4();
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

void ContrastTestTab::drawContrastTestUi() {
    int cols = (int)_manager->_palettes.size();

    if (ImGui::BeginTable("Contrast Settings", cols,
                          ImGuiTableFlags_SizingFixedFit |
                              ImGuiTableFlags_ScrollX)) {
        std::size_t idx = 0;
        for (auto &p : _manager->_palettes) {
            ImGui::TableNextColumn();
            ImGui::PushID((int)idx);
            ImGui::TextUnformatted(p._name.c_str());
            ImGui::PopID();

            drawPalette(p, (int)idx);
            ++idx;
        }
        ImGui::EndTable();
    }
}

void ContrastTestTab::drawPalette(Palette &pal, int pal_idx) {
    ImGui::PushID(std::format("pal-controls-{}", pal_idx).c_str());

    if (ImGui::SmallButton("All fg")) {
        for (auto &sw : pal._swatches)
            sw._fg = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear fg")) {
        for (auto &sw : pal._swatches)
            sw._fg = false;
    }

    if (ImGui::SmallButton("All bg")) {
        for (auto &sw : pal._swatches)
            sw._bg = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear bg")) {
        for (auto &sw : pal._swatches)
            sw._bg = false;
    }
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * 0.05f));
    ImGui::PopID();

    for (std::size_t i = 0; i < pal._swatches.size(); ++i) {
        drawSwatch(pal._swatches[i], pal_idx, (int)i);
    }
}

void ContrastTestTab::drawSwatch(Swatch &sw, int pal_idx, int sw_idx) {
    ImGui::PushID(std::format("{}-{}", pal_idx, sw_idx).c_str());

    ImGui::BeginGroup();
    const std::string picker_id = std::format("picker-{}-{}", pal_idx, sw_idx);
    if (ImGui::ColorButton("##swatch", sw._colour,
                           ImGuiColorEditFlags_NoAlpha |
                               ImGuiColorEditFlags_NoPicker,
                           ImVec2(g_swatchWidthPx, g_swatchHeightPx))) {
        ImGui::OpenPopup(picker_id.c_str());
    }

    if (ImGui::BeginPopup(picker_id.c_str())) {
        if (ImGui::ColorPicker4("##picker", &sw._colour.x,
                                ImGuiColorEditFlags_NoAlpha)) {
            sw._hex = toHexString(sw._colour);
            sw._name = std::format("p{}-{}", pal_idx, sw._hex);
        }
        ImGui::EndPopup();
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextUnformatted(sw._hex.c_str());
    ImGui::Checkbox("fg", &sw._fg);
    ImGui::SameLine();
    ImGui::Checkbox("bg", &sw._bg);
    ImGui::EndGroup();
    ImGui::PopID();
}
ImVec4 ContrastTestTab::getColourFromSwatch(int swIdx,
                                            const ImVec4 &defaultColour) const {
    if (const Swatch *sw = _manager->swatchForIndex(swIdx)) {
        return sw->_colour;
    }
    return defaultColour;
}
} // namespace uc
