// urColo - syntax highlight configuration tab
#include "HighlightsTab.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "../Gui.h"

#include <format>

namespace uc {
// Tab for assigning highlight colours and previewing code snippets.
HighlightsTab::HighlightsTab(GuiManager *manager) : Tab("Highlights", manager) {

    _highlightGroups = {
        {"Comment", "Comments"},
        {"Constant", "any constant"},
        {"String", "\"this is a string\""},
        {"Character", "'c', '\\n'"},
        {"Number", "234, 0xff"},
        {"Boolean", "TRUE, false"},
        {"Float", "2.3e10"},
        {"Identifier", "variable"},
        {"Function", "function()"},
        {"Statement", "return"},
        {"Conditional", "if (x)"},
        {"Repeat", "for"},
        {"Label", "case"},
        {"Operator", "+, *"},
        {"Keyword", "static"},
        {"Exception", "try/catch"},
        {"PreProc", "#define"},
        {"Include", "#include"},
        {"Define", "#define"},
        {"Macro", "MACRO"},
        {"PreCondit", "#if"},
        {"Type", "MyType"},
        {"StorageClass", "static"},
        {"Structure", "struct"},
        {"Typedef", "typedef"},
        {"Special", "special"},
        {"SpecialChar", "\n"},
        {"Delimiter", ";"},
        {"SpecialComment", "TODO"},
        {"Underlined", "underlined"},
        {"Bold", "bold"},
        {"Italic", "italic"},
        {"Error", "error"},
        {"Todo", "TODO"},
        {"javaAnnotation", "@Override"},
    };

    for (std::size_t i = 0; i < _highlightGroups.size(); ++i) {
        _hlIndex[_highlightGroups[i].name] = static_cast<int>(i);
    }

    static const char *sample = R"(
#include <stdio.h>
#define PI 3.14

struct MyType {
    const char* str = "hello";
    int num = 42;
};

int main() {
    // Example loop using the struct
    for (int i = 0; i < num; ++i) {
        if (num > 0 && true) {
            printf("%c\n", 'A');
        } else {
            return 0;
        }
    }
}
    )";

    parseCodeSnippet(sample);
}

// Draw the highlight configuration table and preview area.
void HighlightsTab::drawContent() {
    if (_manager->_palettes.empty()) {
        ImGui::Text("No palettes available.");
        return;
    }

    drawHighlightsTable();
    ImGui::Separator();
    drawCodePreview();
}

// Helper combo box for choosing a swatch index by hex string.
void HighlightsTab::drawSwatchSelector(const std::string label, int &swIdx) {
    const Swatch *currSw = _manager->swatchForIndex(swIdx);
    std::string currLabel = currSw ? currSw->_hex : "None";

    if (ImGui::BeginCombo(label.c_str(), currLabel.c_str())) {
        if (ImGui::Selectable("None", swIdx == -1)) {
            swIdx = -1;
        }

        int idx = 0;
        for (const auto &p : _manager->_palettes) {
            ImGui::Selectable(p._name.c_str(), false,
                              ImGuiSelectableFlags_Disabled);
            for (const auto &sw : p._swatches) {
                bool selected = (swIdx == idx);
                if (ImGui::Selectable(sw._hex.c_str(), selected)) {
                    swIdx = idx;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
                ++idx;
            }
        }
        ImGui::EndCombo();
    }
}

// Render a single code sample using the group's current colours.
void HighlightsTab::drawHighlightExample(const HighlightGroup &hg) {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->ChannelsSplit(3);

    ImVec4 fgColour = ImVec4(1, 1, 1, 1);
    ImVec4 bgColour = ImVec4(0, 0, 0, 0);

    if (const Swatch *globalFg = _manager->swatchForIndex(_globalFgSwatch)) {
        fgColour = globalFg->_colour;
    }

    if (const Swatch *sw = _manager->swatchForIndex(hg.fgSwatch)) {
        fgColour = sw->_colour;
    }

    if (const Swatch *sw = _manager->swatchForIndex(hg.bgSwatch)) {
        bgColour = sw->_colour;
    }

    ImVec2 minPos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(hg.sample.c_str());

    float paddingX = ImGui::GetStyle().CellPadding.x;
    float paddingY = ImGui::GetStyle().CellPadding.y;

    minPos.x += paddingX;
    minPos.y += paddingY;

    // apply global bg first
    if (const Swatch *globalBg = _manager->swatchForIndex(_globalBgSwatch)) {
        dl->ChannelsSetCurrent(0);
        ImVec4 g_bg = globalBg->_colour;

        ImVec2 cellMin = ImGui::GetCursorScreenPos();
        ImVec2 cellMax = {cellMin.x + ImGui::GetContentRegionAvail().x,
                          cellMin.y + textSize.y +
                              ImGui::GetStyle().CellPadding.y * 2};

        dl->AddRectFilled(cellMin, cellMax,
                          ImGui::ColorConvertFloat4ToU32(g_bg));
    }

    if (bgColour.w > 0.0f) {
        dl->ChannelsSetCurrent(1);
        ImVec2 cellMin = ImGui::GetCursorScreenPos();
        ImVec2 cellMax = {cellMin.x + ImGui::GetContentRegionAvail().x,
                          cellMin.y + textSize.y +
                              ImGui::GetStyle().CellPadding.y * 2};
        dl->AddRectFilled(cellMin, cellMax,
                          ImGui::ColorConvertFloat4ToU32(bgColour));
    }

    // draw text
    dl->ChannelsSetCurrent(2);
    ImVec2 textPos = ImGui::GetCursorPos();
    textPos.x += paddingX;
    textPos.y += paddingY;

    ImGui::SetCursorPos(textPos);
    ImGui::PushStyleColor(ImGuiCol_Text, fgColour);
    ImGui::TextUnformatted(hg.sample.c_str());
    ImGui::PopStyleColor();
    dl->ChannelsMerge();
}

// One row of the highlights table containing selectors and preview sample.
void HighlightsTab::drawHighlightGroupRow(HighlightGroup &hg,
                                          std::size_t rowIdx) {
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(hg.name.c_str());

    ImGui::TableSetColumnIndex(1);
    drawHighlightExample(hg);

    ImGui::TableSetColumnIndex(2);
    std::string fgId = std::format("fg##{}", rowIdx);
    drawSwatchSelector(fgId.c_str(), hg.fgSwatch);

    ImGui::TableSetColumnIndex(3);
    std::string bgId = std::format("bg##{}", rowIdx);
    drawSwatchSelector(bgId.c_str(), hg.bgSwatch);
}

// Table listing all highlight groups with colour selectors.
void HighlightsTab::drawHighlightsTable() {
    ImGui::Text("Global Defaults");
    drawSwatchSelector("Default FG", _globalFgSwatch);
    drawSwatchSelector("Default BG", _globalBgSwatch);
    ImGui::Separator();

    if (ImGui::BeginTable("HLTable", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Group");
        ImGui::TableSetupColumn("Example", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("fg");
        ImGui::TableSetupColumn("bg");
        ImGui::TableHeadersRow();

        for (std::size_t i = 0; i < _highlightGroups.size(); ++i) {
            auto &hg = _highlightGroups[i];
            drawHighlightGroupRow(hg, i);
        }
        ImGui::EndTable();
    }
}
ImVec4 HighlightsTab::getColourFromSwatch(int swIdx,
                                          const ImVec4 &defaultColour) const {
    if (const Swatch *sw = _manager->swatchForIndex(swIdx)) {
        return sw->_colour;
    }
    return defaultColour;
}

// Determine the effective colours for a token considering overrides.
HighlightsTab::TokenStyle
HighlightsTab::getTokenStyle(const CodeToken &token) const {
    TokenStyle style;
    // Apply global fg & bg
    if (_globalFgSwatch > -1) {
        style.fgColour = getColourFromSwatch(_globalFgSwatch, style.fgColour);
    } else {
        style.fgColour = ImVec4(1, 1, 1, 1);
    }
    if (_globalBgSwatch > -1) {
        style.bgColour = getColourFromSwatch(_globalBgSwatch, style.bgColour);
    } else {
        style.fgColour = ImVec4(0, 0, 0, 0);
    }

    // Apply token-specific overrides
    if (token.groupIdx >= 0 && token.groupIdx < (int)_highlightGroups.size()) {
        const auto &hg = _highlightGroups[(std::size_t)token.groupIdx];
        style.fgColour = getColourFromSwatch(hg.fgSwatch, style.fgColour);
        style.bgColour = getColourFromSwatch(hg.bgSwatch, style.bgColour);
    }
    return style;
}

// Render a single syntax token using the supplied style.
void HighlightsTab::renderToken(ImDrawList *dl, const CodeToken &token,
                                const TokenStyle &style, ImVec2 &currPos,
                                ImVec2 &overallEnd) {
    ImVec2 tokenSize = ImGui::CalcTextSize(token.text.c_str());
    ImVec2 tokenMin = currPos;
    ImVec2 tokenMax = {currPos.x + tokenSize.x, currPos.y + tokenSize.y};

    // draw token specific bg if any
    if (style.bgColour.w > 0.0f) {
        dl->ChannelsSetCurrent(1);
        dl->AddRectFilled(tokenMin, tokenMax,
                          ImGui::ColorConvertFloat4ToU32(style.bgColour));
    }

    // draw token text
    dl->ChannelsSetCurrent(2);
    dl->AddText(tokenMin, ImGui::ColorConvertFloat4ToU32(style.fgColour),
                token.text.c_str());

    currPos.x += tokenSize.x;
    overallEnd.x = std::max(overallEnd.x, tokenMax.x);
    overallEnd.y = std::max(overallEnd.y, tokenMax.y);
}
// Render the parsed code sample token by token.
void HighlightsTab::renderAllTokens(ImDrawList *dl, ImVec2 &currPos,
                                    ImVec2 &overallEnd) {
    // ImVec2 initialPos = currPos;

    for (const auto &line : _codeSample) {
        ImVec2 lineStart = currPos;
        for (const auto &token : line) {
            TokenStyle style = getTokenStyle(token);
            renderToken(dl, token, style, currPos, overallEnd);
        }

        // move to next line
        currPos.x = lineStart.x;
        currPos.y += ImGui::GetTextLineHeightWithSpacing();
        overallEnd.y = std::max(overallEnd.y, currPos.y);
    }
}

// Draw a background rectangle covering the entire code preview if configured.
void HighlightsTab::renderGlobalBackground(ImDrawList *dl,
                                           const ImVec2 &startPos,
                                           ImVec2 &endPos) {
    if (const Swatch *sw = _manager->swatchForIndex(_globalBgSwatch)) {
        dl->ChannelsSetCurrent(0);
        ImVec4 globalBgCol = sw->_colour;

        if (endPos.y == startPos.y && !_codeSample.empty()) {
            endPos.y = startPos.y + ImGui::GetTextLineHeightWithSpacing();
        }

        dl->AddRectFilled(startPos, endPos,
                          ImGui::ColorConvertFloat4ToU32(globalBgCol));
    }
}

// Tokenise a small snippet of code to use for the preview widget.
void HighlightsTab::parseCodeSnippet(const std::string &code) {
    _codeSample.clear();

    auto id = [&](const std::string &n) {
        auto it = _hlIndex.find(n);
        return it != _hlIndex.end() ? it->second : -1;
    };

    const std::unordered_map<std::string, std::string> tokenMap = {
        {"#include", "Include"},   {"#define", "Define"},
        {"PI", "Macro"},           {"struct", "Structure"},
        {"const", "StorageClass"}, {"char", "Type"},
        {"int", "Type"},           {"float", "Type"},
        {"return", "Statement"},   {"for", "Repeat"},
        {"if", "Conditional"},     {"else", "Conditional"},
        {"true", "Boolean"},       {"false", "Boolean"},
        {"printf", "Function"},    {"main", "Function"},
        {"MyType", "Type"},
    };

    std::istringstream iss(code);
    std::string line;
    while (std::getline(iss, line)) {
        CodeLine parsed;
        std::size_t i = 0;
        while (i < line.size()) {
            char c = line[i];
            if (std::isspace(static_cast<unsigned char>(c))) {
                std::string ws;
                while (i < line.size() &&
                       std::isspace(static_cast<unsigned char>(line[i]))) {
                    ws += line[i++];
                }
                parsed.push_back({ws, -1});
                continue;
            }

            if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                std::string comment = line.substr(i);
                std::size_t todoPos = comment.find("TODO");
                int commentIdx = id("Comment");
                int todoIdx = id("Todo");
                if (todoPos != std::string::npos) {
                    if (todoPos > 0)
                        parsed.push_back(
                            {comment.substr(0, todoPos), commentIdx});
                    parsed.push_back({"TODO", todoIdx});
                    parsed.push_back({comment.substr(todoPos + 4), commentIdx});
                } else {
                    parsed.push_back({comment, commentIdx});
                }
                break;
            }

            if (c == '"') {
                std::string str;
                str += c;
                ++i;
                while (i < line.size()) {
                    str += line[i];
                    if (line[i] == '\\' && i + 1 < line.size()) {
                        ++i;
                        str += line[i];
                    } else if (line[i] == '"') {
                        ++i;
                        break;
                    }
                    ++i;
                }
                parsed.push_back({str, id("String")});
                continue;
            }

            if (c == '\'') {
                std::string chr;
                chr += c;
                ++i;
                while (i < line.size()) {
                    chr += line[i];
                    if (line[i] == '\\' && i + 1 < line.size()) {
                        ++i;
                        chr += line[i];
                    } else if (line[i] == '\'') {
                        ++i;
                        break;
                    }
                    ++i;
                }
                parsed.push_back({chr, id("Character")});
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(c))) {
                std::string num;
                while (i < line.size() &&
                       (std::isdigit(static_cast<unsigned char>(line[i])) ||
                        line[i] == '.' || line[i] == 'e' || line[i] == 'E')) {
                    num += line[i++];
                }
                if (num.find('.') != std::string::npos)
                    parsed.push_back({num, id("Float")});
                else
                    parsed.push_back({num, id("Number")});
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' ||
                c == '#') {
                std::string word;
                while (i < line.size() &&
                       (std::isalnum(static_cast<unsigned char>(line[i])) ||
                        line[i] == '_' || line[i] == '#')) {
                    word += line[i++];
                }
                int idx = id("Identifier");
                auto tm = tokenMap.find(word);
                if (tm != tokenMap.end())
                    idx = id(tm->second);
                parsed.push_back({word, idx});
                continue;
            }

            std::string sym(1, c);
            if (i + 1 < line.size()) {
                char next = line[i + 1];
                if ((c == '+' && next == '+') || (c == '-' && next == '-') ||
                    next == '=' || (c == '&' && next == '&') ||
                    (c == '|' && next == '|')) {
                    sym += next;
                    ++i;
                }
            }
            ++i;

            int idx = id("Operator");
            if (std::string("(){}[];,<>").find(sym[0]) != std::string::npos)
                idx = id("Delimiter");
            parsed.push_back({sym, idx});
        }
        _codeSample.push_back(std::move(parsed));
    }
}

// Render the tokenised code sample with all styling applied.
void HighlightsTab::drawCodePreview() {
    if (_manager->_palettes.empty())
        return;

    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 endPos = startPos;
    ImDrawList *dl = ImGui::GetWindowDrawList();

    // set for 3 layers: global BG, token BG, token FG
    dl->ChannelsSplit(3);

    ImVec2 currDrawPos = startPos;

    renderGlobalBackground(dl, startPos, endPos);
    renderAllTokens(dl, currDrawPos, endPos);

    dl->ChannelsMerge();

    ImVec2 drawnSize = ImVec2(endPos.x - startPos.x, endPos.y - startPos.y);
    drawnSize.x = std::max(0.0f, drawnSize.x);
    drawnSize.y = std::max(0.0f, drawnSize.y);
    ImGui::Dummy(drawnSize);
}
} // namespace uc
