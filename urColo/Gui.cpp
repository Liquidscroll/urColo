#include "Gui.h"
#include "Gui/GenSettingsTab.h"
#include "Gui/HighlightsTab.h"
#include "Gui/PaletteGenTab.h"
#include "Gui/ContrastTestTab.h"
#include "PaletteGenerator.h"
#include <imgui.h>

namespace uc {

void GuiManager::init() {
    _palettes.emplace_back("default");
    _palettes.at(0).addSwatch("p0-#000000", {0.0f, 0.0f, 0.0f, 1.0f});

    _generator = new PaletteGenerator();
    _genSettingsTab = new GenSettingsTab(this, _generator);
    _paletteGenTab = new PaletteGenTab(this, _generator, _genSettingsTab);
    _hlTab = new HighlightsTab(this);
    _contrastTab = new ContrastTestTab(this);
}

void GuiManager::drawTabs() {
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

} // namespace uc
