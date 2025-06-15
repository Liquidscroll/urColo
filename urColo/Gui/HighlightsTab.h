#pragma once

#include "../Colour.h"
#include "Tab.h"
#include "imgui/imgui.h"

namespace uc {
class HighlightsTab : public Tab {
  public:
    HighlightsTab(GuiManager *manager);
    void drawContent() override;
    struct CodeToken {
        std::string text;
        int groupIdx{-1};
    };

  private:
    int _globalFgSwatch{-1};
    int _globalBgSwatch{-1};
    struct HighlightGroup {
        std::string name;
        std::string sample;
        int fgSwatch{-1};
        int bgSwatch{-1};
    };
    std::vector<HighlightGroup> _highlightGroups;
    std::unordered_map<std::string, int> _hlIndex;

    struct TokenStyle {
        ImVec4 fgColour;
        ImVec4 bgColour;
    };
    using CodeLine = std::vector<CodeToken>;
    std::vector<CodeLine> _codeSample;

    void drawHighlightsTable();
    void drawCodePreview();
    void parseCodeSnippet(const std::string &code);
    void drawHighlightExample(const HighlightGroup &hg);
    void drawHighlightGroupRow(HighlightGroup &hg, std::size_t rowIdx);
    ImVec4 getColourFromSwatch(int swIdx, const ImVec4 &defaultColour) const;
    void renderToken(ImDrawList *dl, const CodeToken);
    TokenStyle getTokenStyle(const CodeToken &token) const;
    void renderToken(ImDrawList *dl, const CodeToken &token,
                     const TokenStyle &style, ImVec2 &currPos,
                     ImVec2 &overallEnd);
    void renderAllTokens(ImDrawList *dl, ImVec2 &currPos, ImVec2 &overallEnd);
    void renderGlobalBackground(ImDrawList *dl, const ImVec2 &startPos,
                                ImVec2 &endPos);
    void drawSwatchSelector(const std::string label, int &swIdx);
};
} // namespace uc
