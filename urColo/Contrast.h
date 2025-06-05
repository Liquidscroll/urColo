#pragma once
#include "Colour.h"

namespace uc {
struct ContrastChecker {
    static double ratio(const Colour &fg, const Colour &bg) {
        const double L1 =
            std::max(relativeLuminance(fg), relativeLuminance(bg));
        const double L2 =
            std::min(relativeLuminance(fg), relativeLuminance(bg));

        return (L1 + 0.05) / (L2 + 0.05);
    }

    static bool passesAA(const Colour &fg, const Colour &bg,
                         bool largeText = false) {
        const double need = largeText ? 3.0 : 4.5;
        return ratio(fg, bg) >= need;
    }

    static bool passesAAA(const Colour &fg, const Colour &bg,
                          bool largeText = false) {
        const double need = largeText ? 4.5 : 7.0;
        return ratio(fg, bg) >= need;
    }
};
} // namespace uc
