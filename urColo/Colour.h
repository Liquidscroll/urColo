/**
 * @file
 * @brief Colour primitives and conversion helpers used by urColo.
 *
 * Defines Swatch and Palette types for describing palettes along with
 * simple RGB/LAB structs and the Colour class which holds values in both
 * spaces.  Helper functions perform conversions and JSON serialization.
 */
#pragma once

#include <array>
#include <cstdint>
#include <imgui/imgui.h>

#include <nlohmann/json.hpp>

#include "Logger.h"

using json = nlohmann::json;

//------------------------------------------------------------------------------
// Colour primitives
//------------------------------------------------------------------------------
// Swatch  - a single named colour entry shown in the UI.  A swatch keeps the
//           ImGui RGBA value of the colour, a name used when displaying it and
//           a `_locked` flag used by palette generation.  The optional `_hex`
//           string stores an exported hexadecimal form when saving.  Swatches
//           provide `to_json`/`from_json` helpers for Nlohmann JSON
//           serialization.
//
// Palette - groups multiple `Swatch` objects under a palette name with optional
//           tag strings.  It uses the same JSON helpers so palettes can be
//           written to and restored from disk.
//
// LAB and RGB - minimal structs holding OKLab and linear sRGB components
//               respectively.
//
// Colour - stores a colour in both OKLab (`lab`) and linear sRGB (`rgb`)
//          spaces.  The `alpha` field represents opacity with `1.0` being fully
//          opaque.  Utility functions convert to and from 8-bit sRGB.
//------------------------------------------------------------------------------
namespace uc { // urColo

/// A single named colour entry displayed in the UI.
///
/// Member variables:
/// - `_name`  Name shown to the user.
/// - `_hex`   Exported hexadecimal string when saved.
/// - `_colour` ImGui RGBA value.
/// - `_locked` True when generation should keep the colour.
/// - `_fg`    Indicates the swatch is used as foreground.
/// - `_bg`    Indicates the swatch is used as background.
struct Swatch {
    std::string _name;  ///< Display name
    std::string _hex;   ///< Optional exported hex value
    ImVec4 _colour;     ///< Colour in ImGui format
    bool _locked;       ///< Locked state for palette generation
    bool _fg;           ///< Selected as foreground
    bool _bg;           ///< Selected as background

    /// Construct an empty, unlocked swatch.
    Swatch() = default;

    /// Construct a swatch with a given name and colour.
    ///
    /// \param name   Display name for the colour.
    /// \param colour ImGui colour value.
    Swatch(const std::string &name, const ImVec4 &colour)
        : _name(name), _colour(colour), _locked(false), _fg(false), _bg(false) {
    }

    /// Serialise a swatch to JSON.
    friend void to_json(json &j, const Swatch &s) {
        j = json{
            {"name", s._name},
            {"colour", {s._colour.x, s._colour.y, s._colour.z, s._colour.w}}};
    }

    /// Deserialize a swatch from JSON.
    friend void from_json(const json &j, Swatch &s) {
        // if we're loading the json, swatch wont have the correct colour
        // thus we'll pass a raw JSON with "name": colour
        j.at("name").get_to(s._name);
        auto col = j.at("colour");
        s._colour = ImVec4(col.at(0), col.at(1), col.at(2), col.at(3));
    }
};

/// Collection of swatches grouped under a palette name.
///
/// Member variables:
/// - `_name`     Palette label.
/// - `_swatches` Colours contained in the palette.
struct Palette {
    std::string _name;                 ///< Palette name
    std::vector<Swatch> _swatches;     ///< Stored swatches
    bool _good{false};                 ///< Mark for model training

    /// Construct an empty palette.
    Palette() = default;

    /// Construct a palette with a given name.
    explicit Palette(const std::string &name) : _name(name) {}

    /// Add a new swatch to the palette.
    ///
    /// \param swatchName Name of the swatch to add.
    /// \param colour     Colour value for the swatch.
    void addSwatch(const std::string &swatchName, const ImVec4 &colour) {
        _swatches.emplace_back(swatchName, colour);
    }

    /// Serialise a palette to JSON.
    friend void to_json(json &j, const Palette &p) {
        j = json{{"name", p._name}, {"swatches", p._swatches}, {"good", p._good}};
    }

    /// Deserialize a palette from JSON.
    friend void from_json(const json &j, Palette &p) {
        j.at("name").get_to(p._name);
        j.at("swatches").get_to(p._swatches);
        if (j.contains("good"))
            j.at("good").get_to(p._good);
        else
            p._good = false;
    }
};

/// Components of a colour in OKLab space.
///
/// Member variables: `L`, `a`, `b`.
struct LAB {
    double L{}, a{}, b{};
};
/// Linear RGB colour components.
///
/// Member variables: `r`, `g`, `b`.
struct RGB {
    double r{}, g{}, b{};
};

/// A colour stored in both OKLab and linear sRGB forms.
///
/// Member variables:
/// - `lab`   OKLab components.
/// - `rgb`   Linear sRGB components.
/// - `alpha` Opacity value in range [0,1].
struct Colour {
    LAB lab{};   ///< OKLab representation
    RGB rgb{};   ///< Linear sRGB representation
    double alpha{1.0}; ///< Opacity

    /// Default construct an empty colour (black with alpha 1).
    constexpr Colour() = default;

    /// Construct from 8-bit sRGB components.
    ///
    /// \param r    Red component in the range [0,255].
    /// \param g    Green component in the range [0,255].
    /// \param b    Blue component in the range [0,255].
    /// \param alpha Opacity in the range [0,1].
    /// \return The constructed colour in OKLab space.
    static Colour fromSRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                           double alpha = 1.0) noexcept;

    /// Convert this colour back to 8-bit sRGB.
    ///
    /// \return Array containing the red, green and blue components.
    std::array<std::uint8_t, 3> toSRGB8() const noexcept;

    /// Create a colour from an ImGui vector.
    ///
    /// \param col ImGui RGBA colour value.
    /// \return Colour converted to OKLab space.
    static Colour fromImVec4(const ImVec4 &col) noexcept;

    /// Convert this colour to an ImGui vector.
    ///
    /// \return ImGui RGBA representation of the colour.
    [[nodiscard]] ImVec4 toImVec4() const noexcept;
};

/// Compute WCAG relative luminance for a colour.
///
/// \param sRGB Colour in OKLab/linear space.
/// \return Luminance value in the range [0,1].
double relativeLuminance(const Colour &sRGB);
} // namespace uc
