#include <doctest/doctest.h>
#include "urColo/Colour.h"

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
                uc::Colour c =
                    uc::Colour::fromSRGB(static_cast<std::uint8_t>(r),
                                         static_cast<std::uint8_t>(g),
                                         static_cast<std::uint8_t>(b));
                auto trip = c.toSRGB8();
                CHECK(trip[0] == r);
                CHECK(trip[1] == g);
                CHECK(trip[2] == b);
            }
        }
    }
}

