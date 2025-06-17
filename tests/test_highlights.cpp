#include "urColo/Colour.h"
#include "urColo/Gui/Tab.h"
#include "imgui/imgui.h"
#include <doctest/doctest.h>
#define private public
#include "urColo/Gui/HighlightsTab.h"
#undef private

TEST_CASE("parseCodeSnippet tokenises sample") {
    uc::HighlightsTab tab(nullptr);
    tab.parseCodeSnippet("int x = 1; // TODO fix\n");
    REQUIRE(tab._codeSample.size() == 1);
    const auto &line = tab._codeSample[0];
    auto id = [&](const std::string &n){ return tab._hlIndex[n]; };
    REQUIRE(line.size() == 12);
    CHECK(line[0].text == "int");
    CHECK(line[0].groupIdx == id("Type"));
    CHECK(line[2].text == "x");
    CHECK(line[2].groupIdx == id("Identifier"));
    CHECK(line[4].text == "=");
    CHECK(line[4].groupIdx == id("Operator"));
    CHECK(line[6].text == "1");
    CHECK(line[6].groupIdx == id("Number"));
    CHECK(line[7].text == ";");
    CHECK(line[7].groupIdx == id("Delimiter"));
    CHECK(line[9].text == "// ");
    CHECK(line[9].groupIdx == id("Comment"));
    CHECK(line[10].text == "TODO");
    CHECK(line[10].groupIdx == id("Todo"));
    CHECK(line[11].text == " fix");
    CHECK(line[11].groupIdx == id("Comment"));
}
