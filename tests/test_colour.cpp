// urColo - tests colour conversions and round-trip accuracy
#include "urColo/Colour.h"
#include <doctest/doctest.h>

/**
 * @brief Verify Colour conversion preserves 8-bit values.
 *
 * The OKLab <-> sRGB conversions should round-trip without
 * losing information when starting from exact sRGB bytes.
 */
TEST_CASE("SRGB round trip") {
    for (int r = 0; r <= 255; r += 64) {
        for (int g = 0; g <= 255; g += 64) {
            for (int b = 0; b <= 255; b += 64) {
                uc::Colour c = uc::Colour::fromSRGB(
                    static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g),
                    static_cast<std::uint8_t>(b));
                auto trip = c.toSRGB8();
                CHECK(trip[0] == r);
                CHECK(trip[1] == g);
                CHECK(trip[2] == b);
            }
        }
    }
}

TEST_CASE("fromImVec4 caching round trip") {
    ImVec4 v{0.25f, 0.5f, 0.75f, 1.0f};
    uc::Colour c1 = uc::Colour::fromImVec4(v);
    uc::Colour c2 = uc::Colour::fromImVec4(v); // should use cached result
    auto r1 = c1.toImVec4();
    auto r2 = c2.toImVec4();
    CHECK(r1.x == doctest::Approx(r2.x));
    CHECK(r1.y == doctest::Approx(r2.y));
    CHECK(r1.z == doctest::Approx(r2.z));
    CHECK(r1.w == doctest::Approx(r2.w));
}

TEST_CASE("LCh conversion round trip") {
    uc::LAB lab{0.4, 0.2, -0.1};
    uc::LCh lch = uc::toLCh(lab);
    uc::LAB back = uc::fromLCh(lch);
    CHECK(lab.L == doctest::Approx(back.L));
    CHECK(lab.a == doctest::Approx(back.a));
    CHECK(lab.b == doctest::Approx(back.b));
}
