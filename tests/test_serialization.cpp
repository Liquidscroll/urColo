#include <doctest/doctest.h>
#include "urColo/Colour.h"
#include <nlohmann/json.hpp>

/**
 * @brief Validate palette JSON serialisation and deserialisation.
 *
 * Serialising a palette to JSON and reading it back should
 * reproduce the exact palette data including swatches.
 */
TEST_CASE("palette json round trip") {
    uc::Palette p{"Example"};
    p.addSwatch("a", {0.1f,0.2f,0.3f,1.0f});
    p.addSwatch("b", {0.4f,0.5f,0.6f,1.0f});

    nlohmann::json j = p;
    uc::Palette out = j.get<uc::Palette>();

    CHECK(out._name == p._name);
    CHECK(out._swatches.size() == p._swatches.size());
    for (std::size_t i = 0; i < p._swatches.size(); ++i) {
        CHECK(out._swatches[i]._name == p._swatches[i]._name);
        CHECK(out._swatches[i]._colour.x == doctest::Approx(p._swatches[i]._colour.x));
        CHECK(out._swatches[i]._colour.y == doctest::Approx(p._swatches[i]._colour.y));
        CHECK(out._swatches[i]._colour.z == doctest::Approx(p._swatches[i]._colour.z));
        CHECK(out._swatches[i]._colour.w == doctest::Approx(p._swatches[i]._colour.w));
    }
}

TEST_CASE("palette vector deletion round trip") {
    std::vector<uc::Palette> pals;
    uc::Palette p1{"One"};
    p1.addSwatch("a", {0.1f, 0.2f, 0.3f, 1.0f});
    uc::Palette p2{"Two"};
    p2.addSwatch("b", {0.3f, 0.4f, 0.5f, 1.0f});
    pals.push_back(p1);
    pals.push_back(p2);

    nlohmann::json j = pals;
    auto out = j.get<std::vector<uc::Palette>>();
    CHECK(out.size() == 2);

    out.erase(out.begin());
    nlohmann::json j2 = out;
    auto back = j2.get<std::vector<uc::Palette>>();
    CHECK(back.size() == 1);
    CHECK(back[0]._name == "Two");
}

