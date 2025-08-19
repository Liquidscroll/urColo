// urColo - palette generation tab
#include "PaletteGenTab.h"
#include "Colour.h"
#include "Contrast.h"
#include "Gui/GenSettingsTab.h"
#include "Logger.h"
#include "PaletteGenerator.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "../Gui.h"

#include <GL/gl.h>
#include <format>

namespace uc {
// Tab for interactive palette creation and editing.
PaletteGenTab::PaletteGenTab(GuiManager *manager, PaletteGenerator *generator,
                             GenSettingsTab *settings)
    : Tab("Highlights", manager), _generator(generator), _settings(settings) {}

// Draw palette controls and start/stop generation buttons.
void PaletteGenTab::drawContent() {
    if (_genReady) {
        std::lock_guard<std::mutex> lock(_genMutex);
        if (!_genResult.empty()) {
            _manager->_palettes = std::move(_genResult);
            _genResult.clear();
        }
        _genReady = false;
    }
    ImGui::BeginDisabled(_genRunning);
    if (ImGui::Button("Start Generation")) {
        generate();
    }
    ImGui::EndDisabled();

    if (_genRunning) {
        ImGui::SameLine();
        ImGui::TextUnformatted("Generating...");
    }

    drawPalettes();
}

// Render all palettes in a table with drag and drop support.
void PaletteGenTab::drawPalettes() {
    int cols = (int)_manager->_palettes.size();

    if (ImGui::BeginTable("PaletteTable", cols,
                          ImGuiTableFlags_SizingFixedFit |
                              ImGuiTableFlags_ScrollX)) {
        int idx = 0;
        for (auto &p : _manager->_palettes) {
            ImGui::TableNextColumn();

            ImGui::PushID(idx);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
            ImGui::Button(p._name.c_str(), ImVec2(-FLT_MIN, 0));
            ImGui::PopStyleColor();

            // drag and lock code
            // palettes
            if (ImGui::BeginDragDropSource(
                    ImGuiDragDropFlags_SourceAllowNullID)) {
                int payload = idx;
                ImGui::SetDragDropPayload("UC_PALETTE", &payload,
                                          sizeof(payload));
                ImGui::TextUnformatted(p._name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *pl =
                        ImGui::AcceptDragDropPayload("UC_PALETTE")) {
                    int src = *static_cast<const int *>(pl->Data);
                    _pendingPaletteMoves.push_back({src, idx});
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SetNextItemWidth(kSwatchWidthPx);
            ImGui::InputText("##pal_name", &p._name);

            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(0, 25))) {
                _manager->_palettes.emplace_back(
                    std::format("palette {}", _manager->_palettes.size() + 1));
            }

            if (_manager->_palettes.size() > 1) {
                ImGui::SameLine();
                if (ImGui::Button("X", ImVec2(0, 25))) {
                    _pendingPaletteDeletes.push_back(idx);
                }
            }
            drawPalette(p, idx);
            ImGui::PopID();
            ++idx;
        }

        ImGui::EndTable();
    }
    applyPendingMoves();
}

// Apply any queued drag-and-drop palette or swatch moves.
void PaletteGenTab::applyPendingMoves() {
    if (!_pendingPaletteDeletes.empty()) {
        std::sort(_pendingPaletteDeletes.rbegin(),
                  _pendingPaletteDeletes.rend());
        for (int idx : _pendingPaletteDeletes) {
            if (idx < 0 || idx <= (int)_manager->_palettes.size())
                continue;

            _manager->_palettes.erase(_manager->_palettes.begin() + idx);

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
        if (m.from_pal < 0 || m.from_pal >= (int)_manager->_palettes.size())
            continue;

        if (m.to_pal < 0 || m.to_pal >= (int)_manager->_palettes.size())
            continue;

        auto &src = _manager->_palettes[(std::size_t)m.from_pal];
        auto &dst = _manager->_palettes[(std::size_t)m.to_pal];

        if (m.from_idx < 0 || m.from_idx >= (int)src._swatches.size())
            continue;

        Swatch sw = src._swatches[(std::size_t)m.from_idx];
        src._swatches.erase(src._swatches.begin() + m.from_idx);

        int ins_idx = m.to_idx;
        if (m.from_pal == m.to_pal && m.from_idx < m.to_idx)
            ins_idx -= 1;

        ins_idx = std::clamp(ins_idx, 0, (int)dst._swatches.size());
        dst._swatches.insert(dst._swatches.begin() + ins_idx, sw);
    }
    _pendingMoves.clear();

    for (const auto &pm : _pendingPaletteMoves) {
        if (pm.from_idx < 0 || pm.from_idx >= (int)_manager->_palettes.size())
            continue;
        if (pm.to_idx < 0 || pm.to_idx >= (int)_manager->_palettes.size())
            continue;

        Palette pal = _manager->_palettes[(std::size_t)pm.from_idx];
        _manager->_palettes.erase(_manager->_palettes.begin() + pm.from_idx);

        int ins_idx = pm.to_idx;
        ins_idx = std::clamp(ins_idx, 0, (int)_manager->_palettes.size());
        _manager->_palettes.insert(_manager->_palettes.begin() + ins_idx, pal);
    }
    _pendingPaletteMoves.clear();

    for (std::size_t pal_idx = 0; pal_idx < _manager->_palettes.size();
         ++pal_idx) {
        for (auto &sw : _manager->_palettes[pal_idx]._swatches) {
            if (sw._hex.empty()) {
                sw._hex = toHexString(sw._colour);
            }

            sw._name = std::format("p{}-{}", pal_idx, sw._hex);
        }
    }
}

// Draw a single palette with swatch widgets and drag targets.
void PaletteGenTab::drawPalette(Palette &pal, int pal_idx) {
    ImGui::PushID(std::format("pal-controls-{}", pal_idx).c_str());

    if (ImGui::SmallButton("Lock all")) {
        for (auto &sw : pal._swatches) {
            sw._locked = !sw._locked;
        }
    }

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * 0.5f));
    ImGui::PopID();

    for (std::size_t i = 0; i < pal._swatches.size(); ++i) {
        drawSwatch(pal._swatches[i], pal_idx, (int)i);
    }

    ImGui::PushID(std::format("add-swatch-{}", pal_idx).c_str());
    if (ImGui::Button("+", ImVec2(kSwatchWidthPx, kSwatchHeightPx))) {
        ImVec4 col{0.0f, 0.0f, 0.0f, 1.0f};
        std::string hex = toHexString(col);
        pal.addSwatch(std::format("p{}-{}", pal_idx, hex), col);
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *pl =
                ImGui::AcceptDragDropPayload("UC_SWATCH")) {
            auto data = static_cast<const DragPayload *>(pl->Data);
            _pendingMoves.push_back({data->pal_idx, data->swatch_idx, pal_idx,
                                     static_cast<int>(pal._swatches.size())});
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
}

// Render a draggable colour swatch with hex editing and lock toggle.
void PaletteGenTab::drawSwatch(Swatch &sw, int pal_idx, int sw_idx) {
    constexpr auto flags =
        ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker;

    ImGui::PushID(std::format("{}-{}", pal_idx, sw_idx).c_str());

    ImGui::BeginGroup();
    const std::string picker_id = std::format("picker-{}-{}", pal_idx, sw_idx);

    if (ImGui::ColorButton("##swatch", sw._colour, flags,
                           ImVec2(kSwatchWidthPx, kSwatchHeightPx))) {
        ImGui::OpenPopup(picker_id.c_str());
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        DragPayload pl{pal_idx, sw_idx};
        ImGui::SetDragDropPayload("UC_SWATCH", &pl, sizeof(pl));
        ImGui::Text("%s", sw._name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *pl =
                ImGui::AcceptDragDropPayload("UC_SWATCH")) {
            auto data = static_cast<const DragPayload *>(pl->Data);
            _pendingMoves.push_back(
                {data->pal_idx, data->swatch_idx, pal_idx, sw_idx});
        }
        ImGui::EndDragDropTarget();
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
        std::format("##hex-{}-{}", pal_idx, sw_idx).c_str(), &sw._hex);
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

    if (ImGui::Button(sw._locked ? "unlock" : "lock", ImVec2(0, 25))) {
        sw._locked = !sw._locked;
    }
    ImGui::SameLine();
    ImGui::EndGroup();

    ImGui::PopID();
}

// Perform palette generation either synchronously or in a background thread.
void PaletteGenTab::generate() {

    // startGeneration in Gui.cpp
    if (_genRunning)
        return;

    _generator->clearKMeansImage();

    auto generator = _generator; // copy for thread safety and RNG advance
    auto palettes = _manager->_palettes;
    auto mode = _settings->_genMode;
    auto imgSource = _settings->_imageSource;
    auto imgData = _settings->_imageData;
    int rW = _settings->_randWidth;
    int rH = _settings->_randHeight;

    auto work = [generator, palettes, mode, imgSource, imgData, rW,
                 rH]() mutable {
        if (generator->algorithm() == PaletteGenerator::Algorithm::KMeans) {
            if ((imgSource == GenSettingsTab::ImageSource::Loaded ||
                 imgSource == GenSettingsTab::ImageSource::Random) &&
                !imgData.colours.empty()) {
                generator->setKMeansImage(imgData.colours);
            } else if (imgSource == GenSettingsTab::ImageSource::Random) {
                generator->setKMeansRandomImage(rW, rH);
            } else {
                generator->clearKMeansImage();
            }
        }

        if (mode == GenSettingsTab::GenerationMode::PerPalette) {
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
                    generator->generate(locked, unlocked_indices.size());

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
                auto generated = generator->generate(locked, unlocked.size());
                for (std::size_t i = 0; i < unlocked.size(); ++i) {
                    unlocked[i]->_colour = generated[i]._colour;
                    unlocked[i]->_locked = false;
                }
            }
        }

        return std::make_pair(palettes, generator);
    };

    bool slow = _generator->algorithm() == PaletteGenerator::Algorithm::KMeans;
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
        _manager->_palettes = std::move(result);
        _generator = gen;
    }
}

} // namespace uc
