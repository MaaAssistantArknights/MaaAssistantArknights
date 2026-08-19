#pragma once

#include <array>
#include <cstddef>
#include <optional>

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

[[nodiscard]] constexpr std::optional<FloorProfile> floor_profile(int floor) noexcept
{
    if (floor < 1 || floor > static_cast<int>(FloorProfiles.size())) {
        return std::nullopt;
    }
    return FloorProfiles[static_cast<std::size_t>(floor - 1)];
}
} // namespace asst::blackflow::perception
