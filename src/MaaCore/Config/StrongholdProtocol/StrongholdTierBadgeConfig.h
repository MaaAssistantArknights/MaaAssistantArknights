#pragma once

#include <array>
#include <string_view>

#include <meojson/json.hpp>

namespace asst
{
inline constexpr std::array<std::string_view, 6> StrongholdTierGlyphs { "I", "II", "III", "IV", "V", "VI" };

struct StrongholdTierBadgeConfig
{
    using HsvRange = std::array<std::array<int, 3>, 2>;
    std::array<HsvRange, StrongholdTierGlyphs.size()> hsv {};
    std::array<double, 2> inset {};
    std::array<double, 2> glyph_height {};
    std::array<double, 2> stroke_rows {};
    std::array<double, 2> foreground_fraction {};
    double max_center_offset = 0;
    double max_stroke_width = 0;
    double max_stroke_drift = 0;
    double min_convergence = 0;
    double min_color_fraction = 0;
    double min_contrast = 0;
    double max_background_value = 0;

    bool check_json(const json::value& json) const;
    MEO_FROMJSON(
        hsv,
        inset,
        glyph_height,
        stroke_rows,
        foreground_fraction,
        max_center_offset,
        max_stroke_width,
        max_stroke_drift,
        min_convergence,
        min_color_fraction,
        min_contrast,
        max_background_value);
};
}
