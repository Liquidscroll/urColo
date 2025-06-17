// urColo - generation settings tab
#include "GenSettingsTab.h"
#include "Contrast.h"
#include "Logger.h"
#include "PaletteGenerator.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "../Gui.h"

#include <GL/gl.h>
#include <format>

namespace uc {
// Tab containing algorithm and generation settings.
GenSettingsTab::GenSettingsTab(GuiManager *manager, PaletteGenerator *generator)
    : Tab("Highlights", manager), _generator(generator) {
    _algo = _generator->algorithm();
    _margin = ImGui::GetStyle().FramePadding.x * 2.0f;
    _arrow = ImGui::GetFrameHeight();
}

// Draw all generation options and any image previews.
void GenSettingsTab::drawContent() {
    loadRandomImage();
    loadImage();
    ImGui::TextUnformatted("Gen settings tab not implemented yet.");

    ImGui::SetNextItemWidth(ImGui::CalcTextSize("Random Offset").x + _margin +
                            _arrow);

    drawAlgorithmSelector();

    if (_algo == PaletteGenerator::Algorithm::KMeans) {
        drawKMeansSelectors();
    }

    drawGenModeSelector();
}

// Helper to upload an ImageData object as an OpenGL texture.
unsigned int GenSettingsTab::createTexture(const ImageData &img) {
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

// Poll the async file dialog and kick off image loading.
void GenSettingsTab::loadImage() {
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
}

// Transfer the loaded/random image data into a texture when ready.
void GenSettingsTab::loadRandomImage() {
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
}

// Combo box for selecting per-palette vs global generation.
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

// Combo box for selecting which algorithm generates new colours.
void GenSettingsTab::drawAlgorithmSelector() {
    if (ImGui::BeginCombo("Algorithm", _algNames[std::size_t(_algo)].c_str())) {
        for (int i = 0; std::size_t(i) < _algNames.size(); ++i) {
            bool sel = (i == (int)_algo);
            if (ImGui::Selectable(_algNames[std::size_t(i)].c_str(), sel)) {
                _generator->setAlgorithm(
                    static_cast<PaletteGenerator::Algorithm>(i));
                _algo = _generator->algorithm();
            }

            if (sel)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// Additional widgets shown when the k-means algorithm is active.
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

    drawKMeansImageSelectors();
}

// Show image preview and options for supplying k-means input.
void GenSettingsTab::drawKMeansImageSelectors() {
    // img src is not none and there is an image ready
    if (_imageSource != ImageSource::None && _imageTexture) {
        // draw image
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
                auto img = generateRandomImage(_randWidth, _randHeight);
                _loadedImage = std::move(img);
                _imageReady = true;
            });
            _loadingImage = true;
        }

        if (_loadingImage) {
            drawProgressBar();
        }
    } else if (_imageSource == ImageSource::Loaded) {
        // open file, load img
        if (ImGui::Button("Load Image")) {
            _imageDialog = std::make_unique<pfd::open_file>(
                "Open Image", ".",
                std::vector<std::string>{"Image Files", "*.png *.jpg"});
        }

        if (_loadingImage) {
            drawProgressBar();
        }
    }
}
// Small animated progress bar used while images load in a thread.
void GenSettingsTab::drawProgressBar() {
    // display progress
    ImGui::SameLine();
    static float phase = 0.0f;
    phase += ImGui::GetIO().DeltaTime;
    if (phase < 1.0f) {
        phase -= 1.0f;
    }
    ImGui::ProgressBar(phase, ImVec2(100, 0), "");
}
} // namespace uc
