#pragma once
#include "Colour.h"
#include <random>
#include <vector>

namespace uc {
/// Simple model storing colour statistics learned from palettes.
class Model {
  public:
    explicit Model(std::uint64_t seed = 0);

    /// Update the model using a set of good palettes.
    void train(const std::vector<Palette> &goodPalettes);

    /// Suggest a number of colours sampled from the learned distribution.
    std::vector<Swatch> suggest(std::size_t count);

    /// Serialise model state to JSON.
    friend void to_json(nlohmann::json &j, const Model &m);
    /// Restore model state from JSON.
    friend void from_json(const nlohmann::json &j, Model &m);

  private:
    LAB _mean{};
    LAB _stdev{};
    bool _trained{false};
    std::mt19937_64 _rng;
};
} // namespace uc
