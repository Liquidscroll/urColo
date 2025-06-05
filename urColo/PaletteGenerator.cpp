#include "PaletteGenerator.h"

#include <algorithm>
#include <random>
namespace uc {

PaletteGenerator::PaletteGenerator(std::uint64_t seed)
    : _rng(seed == 0 ? std::random_device{}() : seed) {}

std::vector<Swatch> PaletteGenerator::generate(std::span<const Swatch> locked,
                                               std::size_t want) {
    std::vector<Swatch> output;
    output.reserve(want);

    LAB base{};
    if (!locked.empty()) {
        for (const auto &sw : locked) {
            Colour c = Colour::fromImVec4(sw._colour);
            base.L += c.lab.L;
            base.a += c.lab.a;
            base.b += c.lab.b;
        }
        base.L /= (double)locked.size();
        base.a /= (double)locked.size();
        base.b /= (double)locked.size();
    } else {
        base = {0.5, 0.0, 0.0};
    }

    std::uniform_real_distribution<double> offset(-0.1, 0.1);
    std::uniform_real_distribution<double> luminance(0.0, 1.0);

    for (std::size_t i = 0; i < want; ++i) {
        LAB l{};
        if (!locked.empty()) {
            l.L = std::clamp(base.L + offset(_rng), 0.0, 1.0);
            l.a = base.a + offset(_rng);
            l.b = base.b + offset(_rng);
        } else {
            l.L = luminance(_rng);
            l.a = offset(_rng);
            l.b = offset(_rng);
        }

        Colour col;
        col.lab = l;
        col.alpha = 1.0;
        Swatch sw;
        sw._colour = col.toImVec4();
        sw._locked = false;
        output.push_back(sw);
    }

    return output;
}
} // namespace uc
