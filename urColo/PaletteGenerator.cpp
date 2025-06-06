#include "PaletteGenerator.h"

#include <algorithm>
#include <random>

namespace uc {

PaletteGenerator::PaletteGenerator(std::uint64_t seed)
    : _rng(seed == 0 ? std::random_device{}() : seed) {}

void PaletteGenerator::setKMeansImage(const std::vector<Colour> &img) {
    _kMeansImage.clear();
    _kMeansImage.reserve(img.size());
    for (const auto &c : img) {
        _kMeansImage.push_back(c.lab);
    }
}

void PaletteGenerator::setKMeansRandomImage(int width, int height) {
    _kMeansImage.clear();
    _kMeansImage.reserve(static_cast<std::size_t>(width) * height);
    std::uniform_real_distribution<double> Ld(0.0, 1.0);
    std::uniform_real_distribution<double> ab(-0.5, 0.5);
    for (int i = 0; i < width * height; ++i) {
        _kMeansImage.push_back({Ld(_rng), ab(_rng), ab(_rng)});
    }
}

std::vector<Swatch>
PaletteGenerator::generateRandomOffset(std::span<const Swatch> locked,
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

std::vector<Swatch>
PaletteGenerator::generateKMeans(std::span<const Swatch> locked,
                                 std::size_t want) {
    // Generate sample points from a loaded image or randomly if no image is set,
    // then include locked swatches so they influence clustering.
    const std::size_t randomPoints = 100;
    std::vector<LAB> points;
    if (_kMeansImage.empty()) {
        points.reserve(randomPoints + locked.size());
        std::uniform_real_distribution<double> Ld(0.0, 1.0);
        std::uniform_real_distribution<double> ab(-0.5, 0.5);
        for (std::size_t i = 0; i < randomPoints; ++i) {
            points.push_back({Ld(_rng), ab(_rng), ab(_rng)});
        }
    } else {
        points = _kMeansImage;
        points.reserve(points.size() + locked.size());
    }
    for (const auto &sw : locked) {
        Colour c = Colour::fromImVec4(sw._colour);
        points.push_back(c.lab);
    }

    const std::size_t k = locked.size() + want;
    if (k == 0)
        return {};

    std::vector<LAB> centres;
    std::vector<bool> fixed;
    centres.reserve(k);
    fixed.reserve(k);
    for (const auto &sw : locked) {
        Colour c = Colour::fromImVec4(sw._colour);
        centres.push_back(c.lab);
        fixed.push_back(true); // keep these centres fixed
    }

    // Helper to compute squared distance in OKLab space.

    auto dist2 = [](const LAB &a, const LAB &b) {
        const double dl = a.L - b.L;
        const double da = a.a - b.a;
        const double db = a.b - b.b;
        return dl * dl + da * da + db * db;
    };

    // Choose remaining centres using k-means++.
    std::uniform_int_distribution<std::size_t> pick(0, points.size() - 1);
    if (centres.size() < k) {
        centres.push_back(points[pick(_rng)]);
        fixed.push_back(false);
    }
    while (centres.size() < k) {
        std::vector<double> dist(points.size());
        double sumDist = 0.0;
        for (std::size_t i = 0; i < points.size(); ++i) {
            double best = dist2(points[i], centres[0]);
            for (std::size_t c = 1; c < centres.size(); ++c) {
                double d = dist2(points[i], centres[c]);
                if (d < best)
                    best = d;
            }
            dist[i] = best;
            sumDist += best;
        }
        std::uniform_real_distribution<double> pickDist(0.0, sumDist);
        double target = pickDist(_rng);
        double accum = 0.0;
        std::size_t idx = 0;
        for (; idx < points.size(); ++idx) {
            accum += dist[idx];
            if (accum >= target)
                break;
        }
        if (idx >= points.size())
            idx = points.size() - 1;
        centres.push_back(points[idx]);
        fixed.push_back(false);
    }

    // Perform Lloyd iterations, keeping locked centres fixed.
    for (int iter = 0; iter < _kMeansIterations; ++iter) {
        std::vector<LAB> sum(k);
        std::vector<int> count(k, 0);
        for (const auto &p : points) {
            std::size_t best = 0;
            double bestd = dist2(p, centres[0]);
            for (std::size_t c = 1; c < centres.size(); ++c) {
                double d = dist2(p, centres[c]);
                if (d < bestd) {
                    bestd = d;
                    best = c;
                }
            }
            sum[best].L += p.L;
            sum[best].a += p.a;
            sum[best].b += p.b;
            ++count[best];
        }
        for (std::size_t c = 0; c < centres.size(); ++c) {
            if (!fixed[c] && count[c] > 0) {
                centres[c].L = sum[c].L / count[c];
                centres[c].a = sum[c].a / count[c];
                centres[c].b = sum[c].b / count[c];
            }
        }
    }

    // Return only the centres corresponding to newly generated colours.
    std::vector<Swatch> out;
    out.reserve(want);
    for (std::size_t i = locked.size(); i < centres.size(); ++i) {
        Colour col;
        col.lab = centres[i];
        col.alpha = 1.0;
        Swatch sw;
        sw._colour = col.toImVec4();
        sw._locked = false;
        out.push_back(sw);
    }
    return out;
}

std::vector<Swatch>
PaletteGenerator::generateLearned(std::span<const Swatch> /*locked*/,
                                 std::size_t want) {
    return _model.suggest(want);
}

std::vector<Swatch> PaletteGenerator::generate(std::span<const Swatch> locked,
                                               std::size_t want) {
    switch (_algorithm) {
    case Algorithm::KMeans:
        return generateKMeans(locked, want);
    case Algorithm::Gradient:
        return generateRandomOffset(locked, want);
    case Algorithm::Learned:
        return generateLearned(locked, want);
    case Algorithm::RandomOffset:
    default:
        return generateRandomOffset(locked, want);
    }
}

} // namespace uc
