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

TEST_CASE("gradient interpolation between two locked colours") {
    uc::Swatch start = makeSwatch(255, 0, 0);
    uc::Swatch end = makeSwatch(0, 0, 255);
    std::vector<uc::Swatch> locked{start, end};

    uc::PaletteGenerator gen(123);
    gen.setAlgorithm(uc::PaletteGenerator::Algorithm::Gradient);
    auto result = gen.generate(locked, 2);

    CHECK(result.size() == 2);

    uc::Colour c1 = uc::Colour::fromImVec4(start._colour);
    uc::Colour c2 = uc::Colour::fromImVec4(end._colour);
    if (c1.lab.L > c2.lab.L)
        std::swap(c1, c2);

    auto interp = [&](double t) {
        uc::LAB lab{c1.lab.L + (c2.lab.L - c1.lab.L) * t,
                    c1.lab.a + (c2.lab.a - c1.lab.a) * t,
                    c1.lab.b + (c2.lab.b - c1.lab.b) * t};
        uc::Colour cc;
        cc.lab = lab;
        cc.alpha = 1.0;
        return cc.toImVec4();
    };

    ImVec4 exp1 = interp(1.0 / 3.0);
    ImVec4 exp2 = interp(2.0 / 3.0);

    CHECK(result[0]._colour.x == doctest::Approx(exp1.x).epsilon(0.001));
    CHECK(result[0]._colour.y == doctest::Approx(exp1.y).epsilon(0.001));
    CHECK(result[0]._colour.z == doctest::Approx(exp1.z).epsilon(0.001));
    CHECK(result[1]._colour.x == doctest::Approx(exp2.x).epsilon(0.001));
    CHECK(result[1]._colour.y == doctest::Approx(exp2.y).epsilon(0.001));
    CHECK(result[1]._colour.z == doctest::Approx(exp2.z).epsilon(0.001));
}

