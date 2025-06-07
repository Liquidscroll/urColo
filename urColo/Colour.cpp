/** @file */
#include "Colour.h"
#include "Profiling.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <format>
#include <string>
#include <cctype>

namespace {
using namespace uc;
static std::unordered_map<ImVec4, Colour, ImVec4Hash, ImVec4Equal> cache;
/**
 * @brief Convert an sRGB channel to linear RGB.
 *
 * Uses the ITU-R BT.709 gamma expansion described in the WCAG
 * specification.
 */
inline double SRGBToLinear(double c) {
    if (c <= 0.04045) {
        return c / 12.92;
    } else {
        return std::pow((c + 0.055) / 1.055, 2.4);
    }
}

/**
 * @brief Convert a linear RGB channel to sRGB.
 *
 * This is the inverse operation of @ref SRGBToLinear using the
 * standard sRGB gamma curve.
 */
inline double linearToSRGB(double c) {
    if (c <= 0.0031308) {
        return c * 12.92;
    } else {
        return 1.055 * std::pow(c, 1 / 2.4) - 0.055;
    }
}
/**
 * @brief Convert a linear RGB colour to OKLab.
 *
 * Implementation based on Björn Ottosson's reference code
 * from https://bottosson.github.io/posts/oklab/.
 */
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
/**
 * @brief Convert an OKLab colour back to linear RGB.
 *
 * This is the reverse of @ref LinearToLAB and is also taken from
 * Björn Ottosson's reference implementation.
 */
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

LCh toLCh(const LAB &lab) {
    double C = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    double h = std::atan2(lab.b, lab.a);
    if (h < 0.0)
        h += 2.0 * std::numbers::pi;
    return {lab.L, C, h};
}

LAB fromLCh(const LCh &lch) {
    double a = lch.C * std::cos(lch.h);
    double b = lch.C * std::sin(lch.h);
    return {lch.L, a, b};
}

std::string toHexString(const ImVec4 &c) {
    int r = static_cast<int>(std::round(std::clamp(c.x, 0.0f, 1.0f) * 255.0f));
    int g = static_cast<int>(std::round(std::clamp(c.y, 0.0f, 1.0f) * 255.0f));
    int b = static_cast<int>(std::round(std::clamp(c.z, 0.0f, 1.0f) * 255.0f));
    return std::format("#{:02X}{:02X}{:02X}", r, g, b);
}

bool hexToColour(const std::string &hex, ImVec4 &out) {
    std::string v = hex;
    if (!v.empty() && v[0] == '#')
        v.erase(0, 1);
    if (v.size() != 6)
        return false;
    for (char ch : v) {
        if (!std::isxdigit(static_cast<unsigned char>(ch)))
            return false;
    }
    int value = 0;
    try {
        value = std::stoi(v, nullptr, 16);
    } catch (...) {
        return false;
    }
    int r = (value >> 16) & 0xFF;
    int g = (value >> 8) & 0xFF;
    int b = value & 0xFF;
    out = ImVec4(static_cast<float>(r) / 255.0f,
                 static_cast<float>(g) / 255.0f,
                 static_cast<float>(b) / 255.0f, 1.0f);
    return true;
}
/**
 * @brief Construct a Colour from 8-bit sRGB components.
 *
 * The components are first converted to linear space and then to OKLab.
 * @param r8 Red channel in the range [0,255].
 * @param g8 Green channel in the range [0,255].
 * @param b8 Blue channel in the range [0,255].
 * @param alpha8 Opacity in the range [0,1].
 * @return The resulting Colour.
 */
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

/**
 * @brief Convert this Colour to an 8-bit sRGB triple.
 *
 * The internal OKLab value is converted back to linear RGB and then to sRGB.
 * @return Array containing {r, g, b} in the range [0,255].
 */
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

Colour Colour::fromLCh(const LCh &lch, double alpha) noexcept {
    Colour c;
    c.lab = uc::fromLCh(lch);
    c.rgb = LABToLinear(c.lab);
    c.alpha = alpha;
    return c;
}

/**
 * @brief Create a Colour from an ImVec4.
 *
 * The vector components are interpreted as sRGB values and converted
 * to OKLab.
 */
Colour Colour::fromImVec4(const ImVec4 &v) noexcept {
    auto it = cache.find(v);
    if (it != cache.end())
        return it->second;

    const auto r =
        static_cast<std::uint8_t>(std::clamp(v.x, 0.0f, 1.0f) * 255.0f);
    const auto g =
        static_cast<std::uint8_t>(std::clamp(v.y, 0.0f, 1.0f) * 255.0f);
    const auto b =
        static_cast<std::uint8_t>(std::clamp(v.z, 0.0f, 1.0f) * 255.0f);
    Colour c = fromSRGB(r, g, b, v.w);
    cache.emplace(v, c);
    return c;
}

/**
 * @brief Convert this Colour to an ImVec4.
 *
 * The colour is converted to 8-bit sRGB and packaged in an ImVec4
 * with the stored alpha value.
 */
ImVec4 Colour::toImVec4() const noexcept {
    PROFILE_TO_IMVEC4();
    auto [r8, g8, b8] = toSRGB8();
    return {r8 / 255.0f, g8 / 255.0f, b8 / 255.0f, static_cast<float>(alpha)};
}

/**
 * @brief Compute the WCAG relative luminance for a colour.
 *
 * The colour is converted to sRGB and then to linear space as defined
 * by the WCAG specification.
 * @param c Colour in OKLab/linear space.
 * @return Relative luminance in the range [0,1].
 */
double relativeLuminance(const Colour &c) {
    auto [r8, g8, b8] = c.toSRGB8();
    const double r = SRGBToLinear(r8 / 255.0);
    const double g = SRGBToLinear(g8 / 255.0);
    const double b = SRGBToLinear(b8 / 255.0);
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}
} // namespace uc
