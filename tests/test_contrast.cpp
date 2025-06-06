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
