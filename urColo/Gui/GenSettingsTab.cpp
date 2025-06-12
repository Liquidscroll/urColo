#include "GenSettingsTab.h"
#include "Contrast.h"
#include "Logger.h"
#include "PaletteGenerator.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "../Gui.h"

#include <format>

#define INFO(x) Logger::log(Logger::Level::Info, x)
#define WARN(x) Logger::log(Logger::Level::Warn, x)
#define ERROR(x) Logger::log(Logger::Level::Error, x)
#define OK(x) Logger::log(Logger::Level::Ok, x)

namespace uc {
GenSettingsTab::GenSettingsTab(GuiManager *manager, PaletteGenerator *generator)
    : Tab("Highlights", manager), _generator(generator) {
    INFO("Constructing gen settings tab...");
    _algo = _generator->algorithm();
    INFO(std::format("_algo is: {}", (int)_algo));
    _margin = ImGui::GetStyle().FramePadding.x * 2.0f;
    _arrow = ImGui::GetFrameHeight();
    INFO("Finished constructing gen settings tab.");
}

void GenSettingsTab::drawContent() {
    ImGui::TextUnformatted("Gen settings tab not implemented yet.");

    ImGui::SetNextItemWidth(ImGui::CalcTextSize("Random Offset").x + _margin +
                            _arrow);

    INFO("Before algo selector");
    drawAlgorithmSelector();

    INFO("Before check algo for KMeans");
    if (_algo == PaletteGenerator::Algorithm::KMeans) {
        INFO("Found KMeans algo, drawing selectors...");
        drawKMeansSelectors();
    }

    INFO("Drawing generation mode selectors...");
    drawGenModeSelector();
}

void GenSettingsTab::drawGenModeSelector() {
    int mode = static_cast<int>(_genMode);
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("All Palettes").x + _margin +
                            _arrow);
    if (ImGui::BeginCombo("Generation Mode",
                          _modeNames[std::size_t(mode)].c_str())) {
        for (int i = 0; (std::size_t)i < _modeNames.size(); ++i) {
            bool sel = (i == mode);
            if (ImGui::Selectable(_modeNames[std::size_t(i)].c_str(), sel)) {
                _genMode = static_cast<GenerationMode>(i);
            }

            if (sel)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

void GenSettingsTab::drawAlgorithmSelector() {
    INFO(std::format("size_t _algo: {}", (std::size_t)_algo));
    INFO(std::format("int _algo: {}", (int)_algo));
    INFO(std::format("size_t _algo name: {}", _algNames[(std::size_t)_algo]));
    INFO("Starting Algorithm Combo");
    if (ImGui::BeginCombo("Algorithm", _algNames[(std::size_t)_algo].c_str())) {
        INFO("Before loop.");
        for (int i = 0; std::size_t(i) < _algNames.size(); ++i) {
            INFO(std::format("Loop at {}", i));
            bool sel = (i == (int)_algo);
            if (ImGui::Selectable(_algNames[std::size_t(i)].c_str(), sel)) {
                INFO("Setting algorithm.");
                _generator->setAlgorithm(
                    static_cast<PaletteGenerator::Algorithm>(i));
            }

            if (sel)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
        INFO("End algorithm combo.");
    }
    INFO("Finished drawing algorithm selector.");
}

void GenSettingsTab::drawKMeansSelectors() {
    int iters = _generator->kMeansIterations();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("200").x * 5.0f);
    if (ImGui::DragInt("Iterations", &iters, 1.0f, 1, 200))
        _generator->setKMeansIterations(iters);

    int src = static_cast<int>(_imageSource);
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("Random").x + _margin + _arrow);

    if (ImGui::BeginCombo("KMeans Source",
                          _kmeansSrc[std::size_t(src)].c_str())) {
        for (int i = 0; std::size_t(i) < _kmeansSrc.size(); ++i) {
            bool sel = (i == src);
            if (ImGui::Selectable(_kmeansSrc[std::size_t(i)].c_str(), sel)) {
                _imageSource = static_cast<ImageSource>(i);
            }
            if (sel)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // img src is not none and there is an image ready
    if (_imageSource != ImageSource::None && _imageTexture) {
        // draw image
    }

    if (_imageSource == ImageSource::Random) {
        // draw rand img selectors
    } else if (_imageSource == ImageSource::Loaded) {
        // open file, load img
    }
}
} // namespace uc
