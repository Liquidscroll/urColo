#pragma once
#include "Colour.h"
#include "Gui/ContrastTestTab.h"
#include "Gui/GenSettingsTab.h"
#include "Gui/HighlightsTab.h"
#include "ImageUtils.h"
#include "PaletteGenerator.h"
#include <GLFW/glfw3.h>
#include <atomic>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wshadow"
#include <portable-file-dialogs.h>
#pragma GCC diagnostic pop

namespace uc {

/// Width of colour swatch buttons in pixels.
inline float g_swatchWidthPx = 48.0f;
/// Height of colour swatch buttons in pixels.
inline float g_swatchHeightPx = 48.0f;
/// Manages all ImGui interactions for the application.  This front end
/// initialises ImGui, drives the frame lifecycle and draws palette widgets
/// to the screen.
struct GuiManager {
  public:
    /// Set up the ImGui context for the given window.  Optionally provide the
    /// GLSL version string used by ImGui's OpenGL backend.
    void init(GLFWwindow *wind, const char *glsl_version = "#version 330 core");
    /// Start a new ImGui frame.  Call once per iteration of the render loop.
    void newFrame();
    /// Render the current ImGui draw data to the attached window and swap
    /// buffers.
    void render();
    /// Tear down ImGui and associated backends.
    void shutdown();
    const Swatch *swatchForIndex(int idx) const;

    std::vector<uc::Palette> _palettes;
    struct CodeToken {
        std::string text;
        int groupIdx{-1};
    };
    std::unique_ptr<pfd::open_file> _imageDialog;

  private:
    PaletteGenerator *_generator;
    HighlightsTab *_hlTab;
    ContrastTestTab *_contrastTab;
    GenSettingsTab *_genSettingsTab;

    /// Generation strategy when producing new colours.
    enum class GenerationMode { PerPalette, AllPalettes };
    GenerationMode _genMode{GenerationMode::AllPalettes};

    enum class ImageSource { None, Loaded, Random };
    ImageSource _imageSource{ImageSource::None};
    int _randWidth{64};
    int _randHeight{64};
    ImageData _imageData;   ///< Image used for k-means and preview
    ImageData _loadedImage; ///< Temporary store from loader thread
    unsigned int _imageTexture{0};
    std::jthread _imageThread;
    std::atomic<bool> _loadingImage{false};
    std::atomic<bool> _imageReady{false};

    struct DragPayload {
        int pal_idx;
        int swatch_idx;
    };

    struct PendingMove {
        int from_pal;
        int from_idx;
        int to_pal;
        int to_idx;
    };
    std::vector<PendingMove> _pendingMoves;

    struct PendingPaletteMove {
        int from_idx;
        int to_idx;
    };
    std::vector<PendingPaletteMove> _pendingPaletteMoves;
    /// Indices of palettes queued for deletion.
    std::vector<int> _pendingPaletteDeletes;

    /// Apply queued drag-and-drop operations for swatches and palettes.
    void applyPendingMoves();

    // Background generation state
    std::jthread _genThread;
    std::atomic<bool> _genRunning{false};
    std::atomic<bool> _genReady{false};
    std::mutex _genMutex;
    std::vector<uc::Palette> _genResult;

    void startGeneration();
    /// Draw the palette table containing each palette.
    /// Draw the palette table containing each palette.
    /// @param showFgBg    Display FG/BG controls for each swatch
    /// @param dragAndLock  Allow dragging palettes/swatches and locking colours
    void drawPalettes(bool showFgBg, bool dragAndLock);
    /// Draw an individual palette column.
    /// @param palette palette to display
    /// @param pal_idx index of the palette in the table
    void drawPalette(uc::Palette &palette, int pal_idx, bool showFgBg,
                     bool dragAndLock);
    /// Draw a single colour swatch.
    /// @param sw swatch data being drawn
    /// @param pal_idx owning palette index
    /// @param idx index of the swatch within the palette
    /// @param swatch_width_px width in pixels of the drawn swatch
    /// @param swatch_height_px height in pixels of the drawn swatch
    void drawSwatch(uc::Swatch &sw, int pal_idx, int idx, float swatch_width_px,
                    float swatch_height_px, bool showFgBg, bool dragAndLock);
    /// Draw the highlight groups tab allowing colour assignment
    /// and preview text for each group.
    void drawHighlights();
    /// Draw the sample code snippet using highlight group colours.
    void drawCodePreview();
    /// Parse a sample code snippet into tokens using simple heuristics
    /// and store the result for previewing highlight groups.
    void parseCodeSnippet(const std::string &code);
    GLFWwindow *_window{};
    bool _savePopup{false};
    std::string _lastSavePath;
    std::string _palettePath{"palettes.json"};
    std::unique_ptr<pfd::open_file> _loadDialog;
    std::unique_ptr<pfd::save_file> _paletteSaveDialog;
    std::unique_ptr<pfd::save_file> _modelSaveDialog;
    std::unique_ptr<pfd::open_file> _modelLoadDialog;

    void saveModel(const std::filesystem::path &path);
    void loadModel(const std::filesystem::path &path);

    /// Apply the custom ImGui style and load fonts.
    void applyStyle();

    /// Handler for GLFW error callbacks.
    static void GLFWErrorCallback(int error, const char *desc);
};
} // namespace uc
