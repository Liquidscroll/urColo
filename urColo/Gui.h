#pragma once
#include "Colour.h"
#include "Gui/ContrastTestTab.h"
#include "Gui/GenSettingsTab.h"
#include "Gui/HighlightsTab.h"
#include "Gui/PaletteGenTab.h"
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
    std::unique_ptr<pfd::open_file> _imageDialog;

  private:
    PaletteGenerator *_generator;
    GenSettingsTab *_genSettingsTab;
    PaletteGenTab *_paletteGenTab;
    HighlightsTab *_hlTab;
    ContrastTestTab *_contrastTab;

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
