// urColo - tests image utilities for colour extraction and random generation
#include "urColo/Colour.h"
#include "urColo/ImageUtils.h"
#include <array>
#include <doctest/doctest.h>
#include <string>

TEST_CASE("loadImageColours invalid path") {
    auto cols = uc::loadImageColours("no/such/file.png");
    CHECK(cols.empty());
}

TEST_CASE("loadImageColours returns dominant colours") {
    std::string path = std::string(TEST_ASSETS_DIR) + "/test.png";
    auto cols = uc::loadImageColours(path);
    CHECK(cols.size() == 5);

    std::vector<uc::Colour> expected = {
        uc::Colour::fromSRGB(255, 0, 0), uc::Colour::fromSRGB(0, 255, 0),
        uc::Colour::fromSRGB(0, 0, 255), uc::Colour::fromSRGB(255, 255, 255)};

    for (const auto &exp : expected) {
        bool found = false;
        auto expLch = exp.toLCh();
        for (const auto &c : cols) {
            auto lch = c.toLCh();
            if (doctest::Approx(lch.L).epsilon(0.001) == expLch.L &&
                doctest::Approx(lch.C).epsilon(0.001) == expLch.C &&
                doctest::Approx(lch.h).epsilon(0.001) == expLch.h) {
                found = true;
                break;
            }
        }
        CHECK(found);
    }
}

TEST_CASE("generateRandomImageColours returns five colours") {
    auto cols = uc::generateRandomImageColours(4, 4);
    CHECK(cols.size() == 5);
}

TEST_CASE("loadImageData returns image pixels") {
    std::string path = std::string(TEST_ASSETS_DIR) + "/test.png";
    auto img = uc::loadImageData(path);
    CHECK(img.width == 2);
    CHECK(img.height == 2);
    CHECK(img.rgba.size() == 16);
    CHECK(img.colours.size() == 4);
}

TEST_CASE("generateRandomImage returns requested size") {
    auto img = uc::generateRandomImage(3, 2);
    CHECK(img.width == 3);
    CHECK(img.height == 2);
    CHECK(img.rgba.size() == 24);
    CHECK(img.colours.size() == 6);
}
