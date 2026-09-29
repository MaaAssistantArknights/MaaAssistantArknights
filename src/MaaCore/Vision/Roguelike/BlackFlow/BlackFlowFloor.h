#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>

namespace asst::blackflow::perception
{
struct FloorProfile
{
    int floor = 0;
    int rows = 0;
    int columns = 0;
};

inline constexpr std::array<FloorProfile, 5> FloorProfiles = {
    FloorProfile { 1, 3, 5 }, FloorProfile { 2, 4, 5 },  FloorProfile { 3, 5, 7 },
    FloorProfile { 4, 5, 8 }, FloorProfile { 5, 5, 10 },
};

inline constexpr std::array<FloorProfile, 2> Floor5Profiles = {
    FloorProfile { 5, 5, 10 },
    FloorProfile { 5, 5, 9 },
};

[[nodiscard]] constexpr std::span<const FloorProfile> floor_profiles(int floor) noexcept
{
    if (floor == 5) {
        return Floor5Profiles;
    }
    if (floor < 1 || floor > static_cast<int>(FloorProfiles.size())) {
        return {};
    }
    return std::span<const FloorProfile>(FloorProfiles).subspan(static_cast<std::size_t>(floor - 1), 1);
}

[[nodiscard]] constexpr bool is_supported_floor_grid(int floor, int rows, int columns) noexcept
{
    for (const auto& profile : floor_profiles(floor)) {
        if (profile.rows == rows && profile.columns == columns) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] constexpr std::optional<FloorProfile> floor_profile(int floor) noexcept
{
    if (floor < 1 || floor > static_cast<int>(FloorProfiles.size())) {
        return std::nullopt;
    }
    return FloorProfiles[static_cast<std::size_t>(floor - 1)];
}
} // namespace asst::blackflow::perception
