#include <doctest/doctest.h>
#include "urColo/Colour.h"
#include <nlohmann/json.hpp>

/**
 * @brief Validate palette JSON serialisation and deserialisation.
 *
 * Serialising a palette to JSON and reading it back should
 * reproduce the exact palette data including swatches and tags.
 */
TEST_CASE("palette json round trip") {
    uc::Palette p{"Example"};
    p._tags = {"test", "json"};
    p.addSwatch("a", {0.1f,0.2f,0.3f,1.0f});
    p.addSwatch("b", {0.4f,0.5f,0.6f,1.0f});

    nlohmann::json j = p;
    uc::Palette out = j.get<uc::Palette>();

    CHECK(out._name == p._name);
    CHECK(out._tags == p._tags);
    CHECK(out._swatches.size() == p._swatches.size());
    for (std::size_t i = 0; i < p._swatches.size(); ++i) {
        CHECK(out._swatches[i]._name == p._swatches[i]._name);
        CHECK(out._swatches[i]._colour.x == doctest::Approx(p._swatches[i]._colour.x));
        CHECK(out._swatches[i]._colour.y == doctest::Approx(p._swatches[i]._colour.y));
        CHECK(out._swatches[i]._colour.z == doctest::Approx(p._swatches[i]._colour.z));
        CHECK(out._swatches[i]._colour.w == doctest::Approx(p._swatches[i]._colour.w));
    }
}

