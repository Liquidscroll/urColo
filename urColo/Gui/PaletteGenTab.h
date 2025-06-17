// urColo - palette generation tab interface
#pragma once

#include "../Colour.h"
#include "../ImageUtils.h"
#include "../PaletteGenerator.h"
#include "Gui/GenSettingsTab.h"
#include "Tab.h"
#include "imgui/imgui.h"
#include <thread>
#include "../compiler_warnings.h"
UC_SUPPRESS_WARNINGS_BEGIN
#include <portable-file-dialogs.h>
UC_SUPPRESS_WARNINGS_END
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
