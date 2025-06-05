#pragma once
#include "Colour.h"
#include <random>
#include <span>
#include <vector>

struct PaletteGen {
    std::vector<uc::Colour> generate(std::span<const uc::Colour> locked,
                                     std::size_t want,
                                     std::mt19937_64 &rng) const;
};
