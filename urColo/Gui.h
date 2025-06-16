#pragma once
#include "Colour.h"
#include "Gui/ContrastTestTab.h"
#include "Gui/GenSettingsTab.h"
#include "Gui/HighlightsTab.h"
#include "Gui/PaletteGenTab.h"
#include "ImageUtils.h"
#include "PaletteGenerator.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
    /// Initialise palette data and tabs.
    void init();
    /// Draw all registered tabs inside a tab bar.
    void drawTabs();
    const Swatch *swatchForIndex(int idx) const;

    std::vector<uc::Palette> _palettes;
    std::unique_ptr<pfd::open_file> _imageDialog;

  private:
    friend class WindowManager;
    PaletteGenerator *_generator{};
    GenSettingsTab *_genSettingsTab{};
    PaletteGenTab *_paletteGenTab{};
    HighlightsTab *_hlTab{};
    ContrastTestTab *_contrastTab{};
}; 
} // namespace uc
