#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "urColo/Colour.h"
#include "urColo/Contrast.h"

/**
 * @brief Verify the maximum contrast ratio between black and white.
 *
 * This doctest ensures that a black foreground on a white background
 * produces the expected 21:1 ratio and meets WCAG AAA requirements.
 */

TEST_CASE("black and white contrast") {
    uc::Colour black = uc::Colour::fromSRGB(0, 0, 0);
    uc::Colour white = uc::Colour::fromSRGB(255, 255, 255);
    double ratio = uc::ContrastChecker::ratio(black, white);
    CHECK(doctest::Approx(ratio).epsilon(0.01) == 21.0);
    CHECK(uc::ContrastChecker::passesAAA(black, white));
}

/**
 * @brief Validate AA/AAA checks for grey colour pairs.
 *
 * These tests use mid-range greys to exercise the AA and AAA
 * helper methods with both normal and "large text" thresholds.
 */
TEST_CASE("grey contrast ratios") {
    SUBCASE("fail all thresholds") {
        uc::Colour dark = uc::Colour::fromSRGB(120, 120, 120);
        uc::Colour light = uc::Colour::fromSRGB(200, 200, 200);
        double ratio = uc::ContrastChecker::ratio(dark, light);
        CHECK(doctest::Approx(ratio).epsilon(0.01) == 2.64);
        CHECK_FALSE(uc::ContrastChecker::passesAA(dark, light));
        CHECK_FALSE(uc::ContrastChecker::passesAA(dark, light, true));
        CHECK_FALSE(uc::ContrastChecker::passesAAA(dark, light));
        CHECK_FALSE(uc::ContrastChecker::passesAAA(dark, light, true));
    }

    SUBCASE("pass AA but not AAA") {
        uc::Colour dark = uc::Colour::fromSRGB(50, 50, 50);
        uc::Colour light = uc::Colour::fromSRGB(180, 180, 180);
        double ratio = uc::ContrastChecker::ratio(dark, light);
        CHECK(doctest::Approx(ratio).epsilon(0.01) == 6.18);
        CHECK(uc::ContrastChecker::passesAA(dark, light));
        CHECK(uc::ContrastChecker::passesAA(dark, light, true));
        CHECK_FALSE(uc::ContrastChecker::passesAAA(dark, light));
        CHECK(uc::ContrastChecker::passesAAA(dark, light, true));
    }
}
