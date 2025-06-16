#pragma once

#include "../Colour.h"
#include "../ImageUtils.h"
#include "../PaletteGenerator.h"
#include "Tab.h"
#include "imgui/imgui.h"
#include <thread>
#if defined(__clang__) // ──── Clang ────────────────────────────────
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"

#elif defined(__GNUC__) // ──── GCC / MinGW ─────────────────────────
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wshadow"

#elif defined(_MSC_VER) // ──── MSVC ─────────────────────────────────
// C4668: 'identifier' is not defined as a preprocessor macro
// C4456/4457/4458: declaration of 'x' hides previous local
#pragma warning(push)
#pragma warning(disable : 4668 4456 4457 4458)
#endif

#include <portable-file-dialogs.h> //  third-party header

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
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
    ImageData _imageData; ///< Image used for k-means and preview
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
    ImageData _loadedImage; ///< Temporary store from loader thread

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
