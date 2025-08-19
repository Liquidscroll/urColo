// urColo - tests window manager model persistence and path separator behaviour
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <doctest/doctest.h>
#define private public
#include "urColo/Gui/WindowManager.h"
#include "urColo/Gui.h"
#undef private

struct TestWindowManager : uc::WindowManager {};

namespace uc {
void GuiManager::init() {}
void GuiManager::drawTabs() {}
const uc::Swatch *GuiManager::swatchForIndex(int) const { return nullptr; }
}

TEST_CASE("window manager model save and load") {
    uc::GuiManager gui;
    gui._generator = new uc::PaletteGenerator();
    gui._palettes.clear();
    uc::Palette p{"second"};
    p.addSwatch("a", {1.0f,0.0f,0.0f,1.0f});
    p._good = true;
    gui._palettes.push_back(p);

    TestWindowManager wm;
    wm._gui = &gui;
    auto path = std::filesystem::temp_directory_path() / "wm_model.json";
    std::filesystem::remove(path);

    wm.saveModel(path);

    std::ifstream fin(path);
    REQUIRE(fin.is_open());
    nlohmann::json j; fin >> j;
    fin.close();
    CHECK(j.is_object());
    CHECK(j.contains("mean"));
    CHECK(j.contains("trained"));

    // modify mean and reload
    j["mean"] = {0.1,0.2,0.3};
    std::ofstream fout(path);
    REQUIRE(fout.is_open());
    fout << j.dump(4);
    fout.close();

    wm.loadModel(path);
    nlohmann::json j2 = gui._generator->model();
    auto mean = j2["mean"];
    CHECK(mean[0].get<double>() == doctest::Approx(0.1));
    CHECK(mean[1].get<double>() == doctest::Approx(0.2));
    CHECK(mean[2].get<double>() == doctest::Approx(0.3));

    std::filesystem::remove(path);
    delete gui._generator;
}

TEST_CASE("pfd path separator") {
#ifdef _WIN32
    CHECK(pfd::path::separator() == "\\");
#else
    CHECK(pfd::path::separator() == "/");
#endif
}
