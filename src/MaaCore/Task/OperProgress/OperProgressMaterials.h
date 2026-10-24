#pragma once

#include <charconv>
#include <optional>
#include <string_view>

namespace asst::oper_progress
{
struct MaterialCount
{
    int owned = 0;
    int required = 0;
};

inline std::optional<int> parse_material_number(std::string_view text)
{
    if (text.empty() || text.front() < '0' || text.front() > '9') {
        return std::nullopt;
    }
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size() || value < 0) {
        return std::nullopt;
    }
    return value;
}

inline std::optional<MaterialCount> parse_material_count(std::string_view text)
{
    const auto separator = text.find('/');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }
    const auto owned = parse_material_number(text.substr(0, separator));
    const auto required = parse_material_number(text.substr(separator + 1));
    if (!owned || !required || *required == 0) {
        return std::nullopt;
    }
    return MaterialCount { *owned, *required };
}
}
