#pragma once
#include "Colour.h"
#include "Model.h"
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

    /// Provide pixels for the k-means algorithm from an image.
    void setKMeansImage(const std::vector<Colour> &img);
    /// Generate a random image of the given dimensions for k-means.
    void setKMeansRandomImage(int width, int height);
    /// Clear any previously set image data.
    void clearKMeansImage() { _kMeansImage.clear(); }

    /// Access the underlying learned model.
    [[nodiscard]] Model &model() { return _model; }
    [[nodiscard]] const Model &model() const { return _model; }

  private:
    std::vector<Swatch>
    generateRandomOffset(std::span<const Colour> lockedCols, std::size_t want);
    /// Generate colours using k-means++ seeded by any locked swatches.
    std::vector<Swatch>
    generateKMeans(std::span<const Colour> lockedCols, std::size_t want);
    /// Generate colours using the learned model.
    std::vector<Swatch>
    generateLearned(std::span<const Colour> lockedCols, std::size_t want);
    /// Generate colours by interpolating locked swatches.
    std::vector<Swatch>
    generateGradient(std::span<const Colour> lockedCols, std::size_t want);

    std::mt19937_64 _rng;
    Algorithm _algorithm{Algorithm::RandomOffset};
    int _kMeansIterations{5};
    Model _model;
    std::vector<LAB> _kMeansImage;
};
} // namespace uc
