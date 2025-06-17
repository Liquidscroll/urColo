// urColo - learning model implementation
#include "Model.h"
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace uc {
// Simple statistical model capturing average colour properties.
Model::Model(std::uint64_t seed)
    : _rng(seed == 0 ? std::random_device{}() : seed) {}

// Calculate mean and standard deviation of colours in the given palettes.
void Model::train(const std::vector<Palette> &goodPalettes) {
    std::size_t count = 0;
    LAB mean{};
    LAB m2{}; // sum of squared differences for variance

    for (const auto &pal : goodPalettes) {
        for (const auto &sw : pal._swatches) {
            Colour c = Colour::fromImVec4(sw._colour);
            ++count;
            // Welford's method
            double deltaL = c.lab.L - mean.L;
            double deltaA = c.lab.a - mean.a;
            double deltaB = c.lab.b - mean.b;
            mean.L += deltaL / static_cast<double>(count);
            mean.a += deltaA / static_cast<double>(count);
            mean.b += deltaB / static_cast<double>(count);
            m2.L += deltaL * (c.lab.L - mean.L);
            m2.a += deltaA * (c.lab.a - mean.a);
            m2.b += deltaB * (c.lab.b - mean.b);
        }
    }

    if (count == 0) {
        _trained = false;
        return;
    }

    _mean = mean;
    _stdev.L = std::sqrt(m2.L / static_cast<double>(count));
    _stdev.a = std::sqrt(m2.a / static_cast<double>(count));
    _stdev.b = std::sqrt(m2.b / static_cast<double>(count));
    _trained = true;
}

// Sample new colours from the learned distribution.
std::vector<Swatch> Model::suggest(std::size_t count) {
    std::vector<Swatch> result;
    result.reserve(count);

    std::normal_distribution<double> nL(_mean.L,
                                        _stdev.L > 0 ? _stdev.L : 0.05);
    std::normal_distribution<double> nA(_mean.a,
                                        _stdev.a > 0 ? _stdev.a : 0.05);
    std::normal_distribution<double> nB(_mean.b,
                                        _stdev.b > 0 ? _stdev.b : 0.05);

    std::uniform_real_distribution<double> unifL(0.0, 1.0);
    std::uniform_real_distribution<double> unifA(-0.5, 0.5);

    for (std::size_t i = 0; i < count; ++i) {
        double L, a, b;
        if (_trained) {
            L = std::clamp(nL(_rng), 0.0, 1.0);
            a = std::clamp(nA(_rng), -1.0, 1.0);
            b = std::clamp(nB(_rng), -1.0, 1.0);
        } else {
            L = unifL(_rng);
            a = unifA(_rng);
            b = unifA(_rng);
        }
        Colour c;
        c.lab = {L, a, b};
        c.alpha = 1.0;
        Swatch sw;
        sw._colour = c.toImVec4();
        sw._locked = false;
        result.push_back(sw);
    }
    return result;
}

// Serialise the model to JSON for saving to disk.
void to_json(json &j, const Model &m) {
    j = json{{"mean", {m._mean.L, m._mean.a, m._mean.b}},
             {"stdev", {m._stdev.L, m._stdev.a, m._stdev.b}},
             {"trained", m._trained}};
}

// Restore model state from JSON.
void from_json(const json &j, Model &m) {
    auto mean = j.at("mean");
    m._mean = {mean.at(0), mean.at(1), mean.at(2)};
    auto sd = j.at("stdev");
    m._stdev = {sd.at(0), sd.at(1), sd.at(2)};
    j.at("trained").get_to(m._trained);
}

} // namespace uc
