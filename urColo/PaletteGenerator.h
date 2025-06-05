#pragma once
#include "Colour.h"
#include <random>
#include <span>
#include <vector>

namespace uc {
struct PaletteGenerator {
    std::vector<Swatch> generate(std::span<const Swatch> locked,
                                 std::size_t want, std::mt19937_64 &rng) const;
};
} // namespace uc
