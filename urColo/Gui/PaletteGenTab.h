// urColo - palette generation tab interface
#pragma once

#include "../Colour.h"
#include "../ImageUtils.h"
#include "../PaletteGenerator.h"
#include "Gui/GenSettingsTab.h"
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
class PaletteGenTab : public Tab {
  public:
    PaletteGenTab(GuiManager *manager, PaletteGenerator *generator,
                  GenSettingsTab *settings);
    void drawContent() override;

  private:
    PaletteGenerator *_generator;
    GenSettingsTab *_settings;
    // Background generation state
    std::jthread _genThread;
    std::atomic<bool> _genRunning{false};
    std::atomic<bool> _genReady{false};
    std::mutex _genMutex;
    std::vector<uc::Palette> _genResult;

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
    std::vector<int> _pendingPaletteDeletes;
    void drawPalettes();
    void drawPalette(Palette &pal, int pal_idx);
    void drawSwatch(Swatch &sw, int pal_idx, int sw_idx);
    void applyPendingMoves();
    void generate();
};
} // namespace uc
