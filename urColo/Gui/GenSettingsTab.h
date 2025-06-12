#pragma once

#include "../Colour.h"
#include "../ImageUtils.h"
#include "../PaletteGenerator.h"
#include "Tab.h"
#include "imgui/imgui.h"
#include <thread>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wshadow"
#include <portable-file-dialogs.h>
#pragma GCC diagnostic pop
namespace uc {
class GenSettingsTab : public Tab {
  public:
    GenSettingsTab(GuiManager *manager, PaletteGenerator *generator);
    void drawContent() override;

  private:
    constexpr static const std::array<std::string, 4> _algNames = {
        "Random Offset", "K-Means++", "Gradient", "Learned"};
    constexpr static const std::array<std::string, 3> _kmeansSrc = {
        "None", "Image", "Random"};
    constexpr static const std::array<std::string, 2> _modeNames = {
        "Per Palette", "All Palettes"};

    enum GenerationMode { PerPalette, AllPalettes };
    GenerationMode _genMode{GenerationMode::AllPalettes};

    enum ImageSource { None, Loaded, Random };
    ImageSource _imageSource{ImageSource::None};

    int _randWidth{64};
    int _randHeight{64};
    ImageData _imageData;   ///< Image used for k-means and preview
    ImageData _loadedImage; ///< Temporary store from loader thread
    unsigned int _imageTexture{0};
    std::jthread _imageThread;
    std::atomic<bool> _loadingImage{false};
    std::atomic<bool> _imageReady{false};

    PaletteGenerator *_generator;
    PaletteGenerator::Algorithm _algo;

    float _margin;
    float _arrow;
    void drawGenModeSelector();
    void drawAlgorithmSelector();
    void drawKMeansSelectors();
    void drawKMeansImageSelectors();
    void drawProgressBar();
    void loadImage();
    void loadRandomImage();
    static unsigned int createTexture(const ImageData &img);
    std::unique_ptr<pfd::open_file> _imageDialog;
};
} // namespace uc
