#include "PaletteGenerator.h"
#include "Profiling.h"

#include <algorithm>
#include <numbers>
#include <random>

namespace {
// Weight factors used when generating and clustering colours. Lower weight for
// luminance encourages more saturated results while keeping hue differences
// significant.
constexpr double L_WEIGHT = 0.5;
constexpr double CHROMA_WEIGHT = 1.0;
} // namespace

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
PaletteGenerator::generateRandomOffset(std::span<const Colour> lockedCols,
                                       std::size_t want) {
    std::vector<Swatch> output;
    output.reserve(want);

    LAB baseLab{};
    if (!lockedCols.empty()) {
        for (const auto &c : lockedCols) {
            baseLab.L += c.lab.L;
            baseLab.a += c.lab.a;
            baseLab.b += c.lab.b;
        }
        baseLab.L /= static_cast<double>(lockedCols.size());
        baseLab.a /= static_cast<double>(lockedCols.size());
        baseLab.b /= static_cast<double>(lockedCols.size());
    } else {
        baseLab = {0.5, 0.0, 0.0};
    }
    LCh base = toLCh(baseLab);

    std::uniform_real_distribution<double> offset(-0.1, 0.1);
    std::uniform_real_distribution<double> luminance(0.0, 1.0);
    std::uniform_real_distribution<double> chroma(0.1, 0.6);
    std::uniform_real_distribution<double> hueDist(0.0, 2.0 * std::numbers::pi);

    for (std::size_t i = 0; i < want; ++i) {
        LCh l{};
        if (!lockedCols.empty()) {
            l.L = std::clamp(base.L + offset(_rng) * L_WEIGHT, 0.0, 1.0);
            l.C = std::max(0.0, base.C + offset(_rng) * CHROMA_WEIGHT);
            l.h = base.h + offset(_rng) * CHROMA_WEIGHT;
        } else {
            l.L = luminance(_rng);
            l.C = chroma(_rng);
            l.h = hueDist(_rng);
        }
        if (l.h < 0.0)
            l.h += 2.0 * std::numbers::pi;
        if (l.h >= 2.0 * std::numbers::pi)
            l.h -= 2.0 * std::numbers::pi;

        Colour col = Colour::fromLCh(l);
        col.alpha = 1.0;
        Swatch sw;
        PROFILE_TO_IMVEC4();
        sw._colour = col.toImVec4();
        sw._locked = false;
        output.push_back(sw);
    }

    return output;
}

std::vector<Swatch>
PaletteGenerator::generateKMeans(std::span<const Colour> lockedCols,
                                 std::size_t want) {
    // Generate sample points from a loaded image or randomly if no image is
    // set, then include locked swatches so they influence clustering.
    const std::size_t randomPoints = 100;
    std::vector<LCh> points;
    if (_kMeansImage.empty()) {
        points.reserve(randomPoints + lockedCols.size());
        std::uniform_real_distribution<double> Ld(0.0, 1.0);
        std::uniform_real_distribution<double> Cdist(0.1, 0.6);
        std::uniform_real_distribution<double> hdist(0.0,
                                                     2.0 * std::numbers::pi);
        for (std::size_t i = 0; i < randomPoints; ++i) {
            points.push_back({Ld(_rng), Cdist(_rng), hdist(_rng)});
        }
    } else {
        points.reserve(_kMeansImage.size() + lockedCols.size());
        for (const auto &p : _kMeansImage) {
            points.push_back(toLCh(p));
        }
    }
    for (const auto &c : lockedCols) {
        points.push_back(c.toLCh());
    }

    const std::size_t k = lockedCols.size() + want;
    if (k == 0)
        return {};

    std::vector<LCh> centres;
    std::vector<bool> fixed;
    centres.reserve(k);
    fixed.reserve(k);
    for (const auto &c : lockedCols) {
        centres.push_back(c.toLCh());
        fixed.push_back(true); // keep these centres fixed
    }

    // Helper to compute squared distance in OKLab space.

    auto dist2 = [](const LCh &a, const LCh &b) {
        double dL = (a.L - b.L) * L_WEIGHT;
        double dh = std::remainder(a.h - b.h, 2.0 * std::numbers::pi);
        double term = a.C * a.C + b.C * b.C - 2.0 * a.C * b.C * std::cos(dh);
        return dL * dL + CHROMA_WEIGHT * term;
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
            LAB plab = fromLCh(p);
            sum[best].L += plab.L;
            sum[best].a += plab.a;
            sum[best].b += plab.b;
            ++count[best];
        }
        for (std::size_t c = 0; c < centres.size(); ++c) {
            if (!fixed[c] && count[c] > 0) {
                LAB mean{sum[c].L / count[c], sum[c].a / count[c],
                         sum[c].b / count[c]};
                centres[c] = toLCh(mean);
            }
        }
    }

    // Return only the centres corresponding to newly generated colours.
    std::vector<Swatch> out;
    out.reserve(want);
    for (std::size_t i = lockedCols.size(); i < centres.size(); ++i) {
        Colour col = Colour::fromLCh(centres[i]);
        col.alpha = 1.0;
        Swatch sw;
        PROFILE_TO_IMVEC4();
        sw._colour = col.toImVec4();
        sw._locked = false;
        out.push_back(sw);
    }
    return out;
}

std::vector<Swatch>
PaletteGenerator::generateLearned(std::span<const Colour> /*lockedCols*/,
                                  std::size_t want) {
    return _model.suggest(want);
}

std::vector<Swatch>
PaletteGenerator::generateGradient(std::span<const Colour> lockedCols,
                                   std::size_t want) {
    if (lockedCols.size() < 2)
        return generateRandomOffset(lockedCols, want);

    std::vector<LCh> anchors;
    anchors.reserve(lockedCols.size());
    for (const auto &c : lockedCols) {
        anchors.push_back(c.toLCh());
    }
    std::sort(anchors.begin(), anchors.end(),
              [](const LCh &a, const LCh &b) { return a.L < b.L; });

    std::vector<Swatch> out;
    out.reserve(want);

    const std::size_t n = anchors.size();
    if (want == 0)
        return out;
    const double step =
        static_cast<double>(n - 1) / static_cast<double>(want + 1);
    for (std::size_t i = 1; i <= want; ++i) {
        double pos = step * static_cast<double>(i);
        std::size_t idx = static_cast<std::size_t>(std::floor(pos));
        if (idx >= n - 1)
            idx = n - 2;
        double t = pos - static_cast<double>(idx);
        const LCh &a = anchors[idx];
        const LCh &b = anchors[idx + 1];
        double dh = std::remainder(b.h - a.h, 2.0 * std::numbers::pi);
        LCh lch{a.L + (b.L - a.L) * t, a.C + (b.C - a.C) * t, a.h + dh * t};
        if (lch.h < 0.0)
            lch.h += 2.0 * std::numbers::pi;
        Colour c = Colour::fromLCh(lch);
        c.alpha = 1.0;
        Swatch sw;
        PROFILE_TO_IMVEC4();
        sw._colour = c.toImVec4();
        sw._locked = false;
        out.push_back(sw);
    }
    return out;
}

std::vector<Swatch> PaletteGenerator::generate(std::span<const Swatch> locked,
                                               std::size_t want) {
    std::vector<Colour> lockedCols;
    lockedCols.reserve(locked.size());
    for (const auto &sw : locked) {
        PROFILE_FROM_IMVEC4();
        lockedCols.push_back(Colour::fromImVec4(sw._colour));
    }

    switch (_algorithm) {
    case Algorithm::KMeans:
        return generateKMeans(lockedCols, want);
    case Algorithm::Gradient:
        return generateGradient(lockedCols, want);
    case Algorithm::Learned:
        return generateLearned(lockedCols, want);
    case Algorithm::RandomOffset:
    default:
        return generateRandomOffset(lockedCols, want);
    }
}

} // namespace uc
