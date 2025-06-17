#include "urColo/PaletteGenerator.h"
#include <doctest/doctest.h>
#include <numbers>

// Helper to create a locked swatch from RGB values.
static uc::Swatch makeSwatch(int r, int g, int b) {
    uc::Swatch sw{"",
                  {static_cast<float>(r) / 255.0f,
                   static_cast<float>(g) / 255.0f,
                   static_cast<float>(b) / 255.0f, 1.0f}};
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
    locked.push_back(makeSwatch(10, 20, 30));
    locked.push_back(makeSwatch(200, 150, 100));

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

    auto l1 = c1.toLCh();
    auto l2 = c2.toLCh();

    auto interp = [&](double t) {
        double dh = std::remainder(l2.h - l1.h, 2.0 * std::numbers::pi);
        uc::LCh lch{l1.L + (l2.L - l1.L) * t, l1.C + (l2.C - l1.C) * t,
                    l1.h + dh * t};
        if (lch.h < 0.0)
            lch.h += 2.0 * std::numbers::pi;
        uc::Colour cc = uc::Colour::fromLCh(lch);
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

TEST_CASE("kmeans successive generations differ") {
    uc::PaletteGenerator gen(123);
    gen.setAlgorithm(uc::PaletteGenerator::Algorithm::KMeans);

    auto first = gen.generate({}, 5);
    auto second = gen.generate({}, 5);

    bool same = true;
    for (std::size_t i = 0; i < first.size(); ++i) {
        const auto &a = first[i]._colour;
        const auto &b = second[i]._colour;
        if (a.x != b.x || a.y != b.y || a.z != b.z) {
            same = false;
            break;
        }
    }
    CHECK_FALSE(same);
}

TEST_CASE("kmeans image input produces deterministic output") {
    std::vector<uc::Colour> img{
        uc::Colour::fromSRGB(255, 0, 0),
        uc::Colour::fromSRGB(0, 255, 0),
        uc::Colour::fromSRGB(0, 0, 255),
    };

    uc::PaletteGenerator g1(42);
    g1.setAlgorithm(uc::PaletteGenerator::Algorithm::KMeans);
    g1.setKMeansImage(img);
    auto first = g1.generate({}, 3);

    uc::PaletteGenerator g2(42);
    g2.setAlgorithm(uc::PaletteGenerator::Algorithm::KMeans);
    g2.setKMeansImage(img);
    auto second = g2.generate({}, 3);

    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i]._colour.x == doctest::Approx(second[i]._colour.x));
        CHECK(first[i]._colour.y == doctest::Approx(second[i]._colour.y));
        CHECK(first[i]._colour.z == doctest::Approx(second[i]._colour.z));
    }
}

TEST_CASE("kmeans random image uses generator seed") {
    uc::PaletteGenerator g1(99);
    g1.setAlgorithm(uc::PaletteGenerator::Algorithm::KMeans);
    g1.setKMeansRandomImage(2, 2);
    auto first = g1.generate({}, 2);

    uc::PaletteGenerator g2(99);
    g2.setAlgorithm(uc::PaletteGenerator::Algorithm::KMeans);
    g2.setKMeansRandomImage(2, 2);
    auto second = g2.generate({}, 2);

    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i]._colour.x == doctest::Approx(second[i]._colour.x));
        CHECK(first[i]._colour.y == doctest::Approx(second[i]._colour.y));
        CHECK(first[i]._colour.z == doctest::Approx(second[i]._colour.z));
    }
}

TEST_CASE("learned algorithm forwards to model") {
    uc::PaletteGenerator gen(123);
    gen.setAlgorithm(uc::PaletteGenerator::Algorithm::Learned);
    gen.model() = uc::Model(55);

    uc::Model copy = gen.model();
    auto expected = copy.suggest(4);

    auto result = gen.generate({}, 4);

    REQUIRE(result.size() == expected.size());
    for (std::size_t i = 0; i < result.size(); ++i) {
        CHECK(result[i]._colour.x == doctest::Approx(expected[i]._colour.x));
        CHECK(result[i]._colour.y == doctest::Approx(expected[i]._colour.y));
        CHECK(result[i]._colour.z == doctest::Approx(expected[i]._colour.z));
    }
}
