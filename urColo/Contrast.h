// urColo - contrast utilities
#pragma once
#include "Colour.h"

namespace uc {
constexpr double WCAG_AA_NORMAL = 4.5;
constexpr double WCAG_AA_LARGE = 3.0;
constexpr double WCAG_AAA_NORMAL = 7.0;
constexpr double WCAG_AAA_LARGE = 4.5;
// Utility helpers for WCAG contrast checks.
//
// `ContrastChecker` provides static methods to calculate the contrast ratio
// between two colours and to verify that a colour pair meets the WCAG
// accessibility recommendations.
struct ContrastChecker {
    // Calculate the contrast ratio between the given colours.
    //
    // The ratio is computed from the relative luminance of the foreground and
    // background colours according to the WCAG formula.
    //
    // \param fg Foreground colour.
    // \param bg Background colour.
    // \return The contrast ratio; values >= 1.0 indicate increasing contrast.
    static double ratio(const Colour &fg, const Colour &bg) {
        double L1 = std::max(relativeLuminance(fg), relativeLuminance(bg));
        double L2 = std::min(relativeLuminance(fg), relativeLuminance(bg));

        return (L1 + 0.05) / (L2 + 0.05);
    }

    // Determine if the colours satisfy WCAG AA contrast.
    //
    // \param fg Foreground colour.
    // \param bg Background colour.
    // \param largeText When true, use the "large text" threshold of 3.0;
    // otherwise the threshold is 4.5.
    // \return True if the computed ratio meets or exceeds the threshold.
    static bool passesAA(const Colour &fg, const Colour &bg,
                         bool largeText = false) {
        const double need = largeText ? WCAG_AA_LARGE : WCAG_AA_NORMAL;
        return ratio(fg, bg) >= need;
    }

    // Determine if the colours satisfy WCAG AAA contrast.
    //
    // \param fg Foreground colour.
    // \param bg Background colour.
    // \param largeText When true, use the "large text" threshold of 4.5;
    // otherwise the threshold is 7.0.
    // \return True if the computed ratio meets or exceeds the threshold.
    static bool passesAAA(const Colour &fg, const Colour &bg,
                          bool largeText = false) {
        const double need = largeText ? WCAG_AAA_LARGE : WCAG_AAA_NORMAL;
        return ratio(fg, bg) >= need;
    }
};
} // namespace uc
