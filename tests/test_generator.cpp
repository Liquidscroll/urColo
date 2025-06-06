#include <doctest/doctest.h>
#include "urColo/PaletteGenerator.h"

static uc::Swatch makeSwatch(int r, int g, int b) {
    uc::Swatch sw{"",
                  {static_cast<float>(r) / 255.0f,
                   static_cast<float>(g) / 255.0f,
                   static_cast<float>(b) / 255.0f,
                   1.0f}};
    sw._locked = true;
    return sw;
}

/**
 * @brief Ensure locked swatches remain unchanged during generation.
 *
 * The generator should keep the colour values for any swatch
 * marked as locked while still returning the requested number of
 * swatches in the palette.
 */
TEST_CASE("palette generator preserves locked colours") {
    std::vector<uc::Swatch> locked;
    locked.push_back(makeSwatch(10,20,30));
    locked.push_back(makeSwatch(200,150,100));

    auto copy = locked; // keep original values
    uc::PaletteGenerator gen(123);
    auto result = gen.generate(locked, 3);

    CHECK(result.size() == 3);
    for (std::size_t i = 0; i < locked.size(); ++i) {
        CHECK(locked[i]._colour.x == doctest::Approx(copy[i]._colour.x));
        CHECK(locked[i]._colour.y == doctest::Approx(copy[i]._colour.y));
        CHECK(locked[i]._colour.z == doctest::Approx(copy[i]._colour.z));
        CHECK(locked[i]._colour.w == doctest::Approx(copy[i]._colour.w));
    }
}

