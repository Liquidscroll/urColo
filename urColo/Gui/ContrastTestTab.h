// urColo - contrast testing tab
#pragma once

#include "../Colour.h"
#include "Tab.h"
#include "imgui/imgui.h"

namespace uc {
class ContrastTestTab : public Tab {
  public:
    ContrastTestTab(GuiManager *manager);
    void drawContent() override;

  private:
    struct ContrastResult {
        std::string fgName;
        std::string bgName;
        ImVec4 fgCol{1, 1, 1, 1};
        ImVec4 bgCol{0, 0, 0, 1};
        double ratio{0.0};
        bool aa{false};
        bool aaa{false};
    };
    std::vector<ContrastResult> _contrastResults;
    bool _contrastPopup{false};

    void runContrastTests();
    void drawContrastPopup();
    void drawContrastTestUi();
    void drawPalette(Palette &pal, int pal_idx);
    void drawSwatch(Swatch &sw, int pal_idx, int sw_idx);
    ImVec4 getColourFromSwatch(int swIdx, const ImVec4 &defaultColour) const;
    void drawSwatchSelector(const std::string label, int &swIdx);
};
} // namespace uc
