#pragma once
#include "Colour.h"
#include <random>
#include <span>
#include <vector>

namespace uc {
struct PaletteGenerator {
    explicit PaletteGenerator(std::uint64_t seed = 0);

    std::vector<Swatch> generate(std::span<const Swatch> locked,
                                 std::size_t want);

  private:
    std::mt19937_64 _rng;
};
} // namespace uc
