#include "ImageUtils.h"
#include <stb_image.h>
#include <random>

namespace uc {

std::vector<Colour> loadImageColours(const std::string& path) {
    int w = 0, h = 0, comp = 0;
    unsigned char *data = stbi_load(path.c_str(), &w, &h, &comp, 3);
    if (!data)
        return {};

    std::vector<LAB> points;
    points.reserve(static_cast<std::size_t>(w) * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char *p = data + (y * w + x) * 3;
            Colour c = Colour::fromSRGB(p[0], p[1], p[2], 1.0);
            points.push_back(c.lab);
        }
    }
    stbi_image_free(data);

    const std::size_t k = 5;
    if (points.empty())
        return {};

    std::mt19937_64 rng{std::random_device{}()};

    auto dist2 = [](const LAB &a, const LAB &b) {
        double dl = a.L - b.L;
        double da = a.a - b.a;
        double db = a.b - b.b;
        return dl * dl + da * da + db * db;
    };

    std::vector<LAB> centres;
    centres.reserve(k);
    std::uniform_int_distribution<std::size_t> pick(0, points.size() - 1);
    centres.push_back(points[pick(rng)]);
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
        double target = pickDist(rng);
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
    }

    int iterations = 5;
    for (int iter = 0; iter < iterations; ++iter) {
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
            if (count[c] > 0) {
                centres[c].L = sum[c].L / count[c];
                centres[c].a = sum[c].a / count[c];
                centres[c].b = sum[c].b / count[c];
            }
        }
    }

    std::vector<Colour> out;
    out.reserve(k);
    for (const auto &lab : centres) {
        Colour col;
        col.lab = lab;
        col.alpha = 1.0;
        out.push_back(col);
    }
    return out;
}

} // namespace uc
