#pragma once

#include <array>
#include <cstdint>
#include <imgui/imgui.h>

#include <nlohmann/json.hpp>

#include "Logger.h"

using json = nlohmann::json;
namespace uc { // urColo

struct Swatch {
    std::string _name;
    std::string _hex;
    ImVec4 _colour;
    bool _locked;
    bool _fg;
    bool _bg;

    Swatch() = default;
    Swatch(const std::string &name, const ImVec4 &colour)
        : _name(name), _colour(colour), _locked(false), _fg(false), _bg(false) {
    }

    friend void to_json(json &j, const Swatch &s) {
        j = json{
            {"name", s._name},
            {"colour", {s._colour.x, s._colour.y, s._colour.z, s._colour.w}}};
    }

    friend void from_json(const json &j, Swatch &s) {
        // if we're loading the json, swatch wont have the correct colour
        // thus we'll pass a raw JSON with "name": colour
        j.at("name").get_to(s._name);
        auto col = j.at("colour");
        s._colour = ImVec4(col.at(0), col.at(1), col.at(2), col.at(3));
    }
};

struct Palette {
    std::string _name;
    std::vector<std::string> _tags;
    std::vector<Swatch> _swatches;

    Palette() = default;
    Palette(const std::string &name) : _name(name) {}

    void addSwatch(const std::string &swatchName, const ImVec4 &colour) {
        _swatches.emplace_back(swatchName, colour);
    }

    friend void to_json(json &j, const Palette &p) {
        j = json{
            {"name", p._name}, {"tags", p._tags}, {"swatches", p._swatches}};
    }

    friend void from_json(const json &j, Palette &p) {
        j.at("name").get_to(p._name);
        j.at("tags").get_to(p._tags);
        j.at("swatches").get_to(p._swatches);
    }
};

struct LAB {
    double L{}, a{}, b{};
};
struct RGB {
    double r{}, g{}, b{};
};

struct Colour {
    LAB lab{}; // OKLab
    RGB rgb{}; // srgb
    double alpha{1.0};

    constexpr Colour() = default;
    // Construct a colour from 8‑bit sRGB components. The values are converted
    // to linear space and then to OKLab. Returns the resulting colour.
    static Colour fromSRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                           double alpha = 1.0) noexcept;
    // Convert this colour back to 8‑bit sRGB for display.
    std::array<std::uint8_t, 3> toSRGB8() const noexcept;
    static Colour fromImVec4(const ImVec4 &col) noexcept;
    [[nodiscard]] ImVec4 toImVec4() const noexcept;
};

// Compute WCAG relative luminance from a colour in OKLab/linear space.
// Returns a value in the range [0, 1].
double relativeLuminance(const Colour &sRGB);
} // namespace uc
