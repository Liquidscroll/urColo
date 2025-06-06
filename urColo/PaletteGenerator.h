#pragma once
#include "Colour.h"
#include <random>
#include <span>
#include <vector>

namespace uc {
struct PaletteGenerator {
    /// Available palette generation algorithms.
    enum class Algorithm { RandomOffset, KMeans, Gradient, Learned };

    explicit PaletteGenerator(std::uint64_t seed = 0);

    /// Generate `want` colours using the currently selected algorithm.
    std::vector<Swatch> generate(std::span<const Swatch> locked,
                                 std::size_t want);

    /// Set the algorithm used when generating colours.
    void setAlgorithm(Algorithm alg) { _algorithm = alg; }
    /// Retrieve the currently active algorithm.
    [[nodiscard]] Algorithm algorithm() const { return _algorithm; }

    /// Set the number of iterations used by the k-means algorithm.
    void setKMeansIterations(int it) { _kMeansIterations = it; }
    /// Get the currently configured number of k-means iterations.
    [[nodiscard]] int kMeansIterations() const { return _kMeansIterations; }

  private:
    std::vector<Swatch> generateRandomOffset(std::span<const Swatch> locked,
                                             std::size_t want);
    /// Generate colours using k-means++ seeded by any locked swatches.
    std::vector<Swatch> generateKMeans(std::span<const Swatch> locked,
                                       std::size_t want);

    std::mt19937_64 _rng;
    Algorithm _algorithm{Algorithm::RandomOffset};
    int _kMeansIterations{5};
};
} // namespace uc
