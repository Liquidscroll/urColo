#include "Colour.h"
#include <algorithm>
#include <cmath>

namespace {
using namespace uc;
inline double SRGBToLinear(double c) {
    if (c <= 0.04045) {
        return c / 12.92;
    } else {
        return std::pow((c + 0.055) / 1.055, 2.4);
    }
}

inline double linearToSRGB(double c) {
    if (c <= 0.0031308) {
        return c * 12.92;
    } else {
        return 1.055 * std::pow(c, 1 / 2.4) - 0.055;
    }
}
// These functions are from: https://bottosson.github.io/posts/oklab/
inline LAB LinearToLAB(RGB c) {
    double l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
    double m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
    double s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    double l_ = cbrt(l);
    double m_ = cbrt(m);
    double s_ = cbrt(s);

    return {
        0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
        1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
        0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
    };
}
inline RGB LABToLinear(LAB c) {
    double l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
    double m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
    double s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

    double l = l_ * l_ * l_;
    double m = m_ * m_ * m_;
    double s = s_ * s_ * s_;

    return {
        +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
        -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
        -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
    };
}
} // namespace

namespace uc {
Colour Colour::fromSRGB(std::uint8_t r8, std::uint8_t g8, std::uint8_t b8,
                        double alpha8) noexcept {
    const double r = SRGBToLinear(r8 / 255.0);
    const double g = SRGBToLinear(g8 / 255.0);
    const double b = SRGBToLinear(b8 / 255.0);

    Colour c;
    c.rgb.r = r;
    c.rgb.g = g;
    c.rgb.b = b;

    c.lab = LinearToLAB(c.rgb);
    c.alpha = alpha8;
    return c;
}

std::array<std::uint8_t, 3> Colour::toSRGB8() const noexcept {
    auto linear = LABToLinear(lab);
    double r, g, b;
    r = linearToSRGB(linear.r);
    g = linearToSRGB(linear.g);
    b = linearToSRGB(linear.b);

    return {
        static_cast<std::uint8_t>(std::lround(std::clamp(r, 0.0, 1.0) * 255.0)),
        static_cast<std::uint8_t>(std::lround(std::clamp(g, 0.0, 1.0) * 255.0)),
        static_cast<std::uint8_t>(std::lround(std::clamp(b, 0.0, 1.0) * 255.0)),
    };
}

Colour Colour::fromImVec4(const ImVec4 &v) noexcept {
    const auto r =
        static_cast<std::uint8_t>(std::clamp(v.x, 0.0f, 1.0f) * 255.0f);
    const auto g =
        static_cast<std::uint8_t>(std::clamp(v.y, 0.0f, 1.0f) * 255.0f);
    const auto b =
        static_cast<std::uint8_t>(std::clamp(v.z, 0.0f, 1.0f) * 255.0f);
    return fromSRGB(r, g, b, v.w);
}

ImVec4 Colour::toImVec4() const noexcept {
    auto [r8, g8, b8] = toSRGB8();
    return {r8 / 255.0f, g8 / 255.0f, b8 / 255.0f, static_cast<float>(alpha)};
}

double relativeLuminance(const Colour &c) {
    auto [r8, g8, b8] = c.toSRGB8();
    const double r = SRGBToLinear(r8 / 255.0);
    const double g = SRGBToLinear(g8 / 255.0);
    const double b = SRGBToLinear(b8 / 255.0);
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}
} // namespace uc
