// urColo - generation settings tab interface
#pragma once

#include "../Colour.h"
#include "../ImageUtils.h"
#include "../PaletteGenerator.h"
#include "Tab.h"
#include "imgui/imgui.h"
#include <thread>
#include "../compiler_warnings.h"
UC_SUPPRESS_WARNINGS_BEGIN
#include <portable-file-dialogs.h>
UC_SUPPRESS_WARNINGS_END
namespace uc {
class GenSettingsTab : public Tab {
  public:
    GenSettingsTab(GuiManager *manager, PaletteGenerator *generator);
    void drawContent() override;

    int _randWidth{64};
    int _randHeight{64};
    enum GenerationMode { PerPalette, AllPalettes };
    GenerationMode _genMode{GenerationMode::AllPalettes};

    enum ImageSource { None, Loaded, Random };
    ImageSource _imageSource{ImageSource::None};
    ImageData _imageData; //< Image used for k-means and preview
  private:
    static inline const std::array<std::string, 4> _algNames = {
        "Random Offset", "K-Means++", "Gradient", "Learned"};
    static inline const std::array<std::string, 3> _kmeansSrc = {
        "None", "Image", "Random"};
    static inline const std::array<std::string, 2> _modeNames = {
        "Per Palette", "All Palettes"};

    unsigned int _imageTexture{0};
    std::jthread _imageThread;
    std::atomic<bool> _loadingImage{false};
    std::atomic<bool> _imageReady{false};
    ImageData _loadedImage; //< Temporary store from loader thread

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
