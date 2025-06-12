#include "Gui.h"

#include "Gui/GenSettingsTab.h"
#include "Gui/HighlightsTab.h"
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

static unsigned int createTexture(const ImageData &img) {
    if (img.rgba.empty())
        return 0;
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, img.rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
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

    _hlTab = new HighlightsTab(this);
    _contrastTab = new ContrastTestTab(this);
    _genSettingsTab = new GenSettingsTab(this, _generator);
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

    _generator->clearKMeansImage();

    auto generator = _generator; // copy for thread safety and RNG advance
    auto palettes = _palettes;
    auto mode = _genMode;
    auto imgSource = _imageSource;
    auto imgData = _imageData;
    int rW = _randWidth;
    int rH = _randHeight;

    auto work = [generator, palettes, mode, imgSource, imgData, rW,
                 rH]() mutable {
        if (generator->algorithm() == PaletteGenerator::Algorithm::KMeans) {
            if ((imgSource == ImageSource::Loaded ||
                 imgSource == ImageSource::Random) &&
                !imgData.colours.empty()) {
                generator->setKMeansImage(imgData.colours);
            } else if (imgSource == ImageSource::Random) {
                generator->setKMeansRandomImage(rW, rH);
            } else {
                generator->clearKMeansImage();
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
    if (_genReady) {
        std::lock_guard<std::mutex> lock(_genMutex);
        if (!_genResult.empty()) {
            _palettes = std::move(_genResult);
            _genResult.clear();
        }
        _genReady = false;
    }
    if (_imageReady) {
        _imageData = std::move(_loadedImage);
        _loadedImage = {};
        if (_imageTexture) {
            glDeleteTextures(1, &_imageTexture);
            _imageTexture = 0;
        }
        _imageTexture = createTexture(_imageData);
        _loadingImage = false;
        _imageReady = false;
        if (_imageThread.joinable())
            _imageThread.join();
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
    if (_imageDialog && _imageDialog->ready()) {
        auto paths = _imageDialog->result();
        _imageDialog.reset();
        if (!paths.empty()) {
            auto path = paths[0];
            _imageThread = std::jthread([this, path]() {
                auto img = loadImageData(path);
                _loadedImage = std::move(img);
                _imageReady = true;
            });
            _loadingImage = true;
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
            ImGui::BeginDisabled(_genRunning || _loadingImage);
            if (ImGui::Button("start generation")) {
                startGeneration();
            }
            ImGui::EndDisabled();
            if (_genRunning) {
                ImGui::SameLine();
                ImGui::TextUnformatted("Generating...");
            } else if (_loadingImage) {
                ImGui::SameLine();
                static float phase = 0.0f;
                phase += ImGui::GetIO().DeltaTime;
                if (phase > 1.0f)
                    phase -= 1.0f;
                ImGui::ProgressBar(phase, ImVec2(100, 0), "");
            }
            drawPalettes(false, true);
            ImGui::EndTabItem();
        }
        // TODO [GEN-SET]: Remove the below
        if (ImGui::BeginTabItem("Old Generation Settings")) {
            static const char *algNames[] = {"Random Offset", "K-Means++",
                                             "Gradient", "Learned"};
            auto alg = _generator->algorithm();

            const float margin = ImGui::GetStyle().FramePadding.x * 2.0f;
            const float arrow = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("Random Offset").x +
                                    margin + arrow);
            if (ImGui::BeginCombo("Algorithm", algNames[(int)alg])) {
                for (int i = 0; i < 4; ++i) {
                    bool sel = i == (int)alg;
                    if (ImGui::Selectable(algNames[i], sel))
                        _generator->setAlgorithm(
                            static_cast<PaletteGenerator::Algorithm>(i));
                    if (sel)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (alg == PaletteGenerator::Algorithm::KMeans) {
                int it = _generator->kMeansIterations();
                ImGui::SetNextItemWidth(ImGui::CalcTextSize("200").x * 5.0f);
                if (ImGui::DragInt("Iterations", &it, 1.0f, 1, 200))
                    _generator->setKMeansIterations(it);
                // CHECKPOINT1
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
                if (_imageSource != ImageSource::None && _imageTexture) {
                    ImGui::SameLine();
                    float h = g_swatchHeightPx * 1.5f;
                    float aspect = static_cast<float>(_imageData.width) /
                                   static_cast<float>(_imageData.height);
                    ImGui::Image(static_cast<ImTextureID>(_imageTexture),
                                 ImVec2(h * aspect, h));
                }
                if (_imageSource == ImageSource::Random) {
                    const float field = ImGui::CalcTextSize("512").x * 5.0f;
                    ImGui::SetNextItemWidth(field);
                    ImGui::DragInt("Width", &_randWidth, 1.0f, 1, 512);
                    ImGui::SetNextItemWidth(field);
                    ImGui::DragInt("Height", &_randHeight, 1.0f, 1, 512);
                    if (ImGui::Button("Generate Image")) {
                        _imageThread = std::jthread([this]() {
                            auto img =
                                generateRandomImage(_randWidth, _randHeight);
                            _loadedImage = std::move(img);
                            _imageReady = true;
                        });
                        _loadingImage = true;
                    }
                    if (_loadingImage) {
                        ImGui::SameLine();
                        static float phase = 0.0f;
                        phase += ImGui::GetIO().DeltaTime;
                        if (phase > 1.0f)
                            phase -= 1.0f;
                        ImGui::ProgressBar(phase, ImVec2(100, 0), "");
                    }
                } else if (_imageSource == ImageSource::Loaded) {
                    if (ImGui::Button("Load Image")) {
                        _imageDialog = std::make_unique<pfd::open_file>(
                            "Open Image", ".",
                            std::vector<std::string>{"Image Files",
                                                     "*.png *.jpg *.bmp"});
                    }
                    if (_loadingImage) {
                        ImGui::SameLine();
                        static float phase = 0.0f;
                        phase += ImGui::GetIO().DeltaTime;
                        if (phase > 1.0f)
                            phase -= 1.0f;
                        ImGui::ProgressBar(phase, ImVec2(100, 0), "");
                    }
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

        auto &src = _palettes[(std::size_t)m.from_pal];
        auto &dst = _palettes[(std::size_t)m.to_pal];
        if (m.from_idx < 0 || m.from_idx >= (int)src._swatches.size()) {
            continue;
        }

        uc::Swatch sw = src._swatches[(std::size_t)m.from_idx];
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

        uc::Palette pal = _palettes[(std::size_t)pm.from_idx];
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
