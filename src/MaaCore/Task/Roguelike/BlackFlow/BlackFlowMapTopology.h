#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace asst::blackflow
{
enum class MapTopology
{
    Floor1HorizontalFork,
    Floor1SingleExit,
    Floor1VerticalFork,
    Floor201,
    Floor202,
    Floor203,
    Floor204,
    Floor205,
    Floor206,
    Floor207,
    Floor208,
    Floor209,
    Floor210,
    Floor301,
    Floor302,
    Floor303,
    Floor304,
    Floor305,
    Floor306,
    Floor307,
    Floor308,
    Floor309,
    Floor310,
    Floor401,
    Floor403,
    Floor404,
    Floor405,
    Floor406,
    Floor407,
    Floor408,
    Floor409,
    Floor410,
    Floor411,
    Floor501,
    Floor502,
    Floor503,
    Floor504,
    Floor505,
    Floor506,
    Floor507,
    Floor508,
    Floor509,
    Floor510,
};

inline constexpr std::array MapTopologyNames = {
    std::pair { MapTopology::Floor1HorizontalFork, std::string_view("floor-1-horizontal-fork") },
    std::pair { MapTopology::Floor1SingleExit, std::string_view("floor-1-single-exit") },
    std::pair { MapTopology::Floor1VerticalFork, std::string_view("floor-1-vertical-fork") },
    std::pair { MapTopology::Floor201, std::string_view("floor-2-01") },
    std::pair { MapTopology::Floor202, std::string_view("floor-2-02") },
    std::pair { MapTopology::Floor203, std::string_view("floor-2-03") },
    std::pair { MapTopology::Floor204, std::string_view("floor-2-04") },
    std::pair { MapTopology::Floor205, std::string_view("floor-2-05") },
    std::pair { MapTopology::Floor206, std::string_view("floor-2-06") },
    std::pair { MapTopology::Floor207, std::string_view("floor-2-07") },
    std::pair { MapTopology::Floor208, std::string_view("floor-2-08") },
    std::pair { MapTopology::Floor209, std::string_view("floor-2-09") },
    std::pair { MapTopology::Floor210, std::string_view("floor-2-10") },
    std::pair { MapTopology::Floor301, std::string_view("floor-3-01") },
    std::pair { MapTopology::Floor302, std::string_view("floor-3-02") },
    std::pair { MapTopology::Floor303, std::string_view("floor-3-03") },
    std::pair { MapTopology::Floor304, std::string_view("floor-3-04") },
    std::pair { MapTopology::Floor305, std::string_view("floor-3-05") },
    std::pair { MapTopology::Floor306, std::string_view("floor-3-06") },
    std::pair { MapTopology::Floor307, std::string_view("floor-3-07") },
    std::pair { MapTopology::Floor308, std::string_view("floor-3-08") },
    std::pair { MapTopology::Floor309, std::string_view("floor-3-09") },
    std::pair { MapTopology::Floor310, std::string_view("floor-3-10") },
    std::pair { MapTopology::Floor401, std::string_view("floor-4-01") },
    std::pair { MapTopology::Floor403, std::string_view("floor-4-03") },
    std::pair { MapTopology::Floor404, std::string_view("floor-4-04") },
    std::pair { MapTopology::Floor405, std::string_view("floor-4-05") },
    std::pair { MapTopology::Floor406, std::string_view("floor-4-06") },
    std::pair { MapTopology::Floor407, std::string_view("floor-4-07") },
    std::pair { MapTopology::Floor408, std::string_view("floor-4-08") },
    std::pair { MapTopology::Floor409, std::string_view("floor-4-09") },
    std::pair { MapTopology::Floor410, std::string_view("floor-4-10") },
    std::pair { MapTopology::Floor411, std::string_view("floor-4-11") },
    std::pair { MapTopology::Floor501, std::string_view("floor-5-01") },
    std::pair { MapTopology::Floor502, std::string_view("floor-5-02") },
    std::pair { MapTopology::Floor503, std::string_view("floor-5-03") },
    std::pair { MapTopology::Floor504, std::string_view("floor-5-04") },
    std::pair { MapTopology::Floor505, std::string_view("floor-5-05") },
    std::pair { MapTopology::Floor506, std::string_view("floor-5-06") },
    std::pair { MapTopology::Floor507, std::string_view("floor-5-07") },
    std::pair { MapTopology::Floor508, std::string_view("floor-5-08") },
    std::pair { MapTopology::Floor509, std::string_view("floor-5-09") },
    std::pair { MapTopology::Floor510, std::string_view("floor-5-10") },
};

[[nodiscard]] constexpr std::string_view to_string(MapTopology topology) noexcept
{
    for (const auto& [value, name] : MapTopologyNames) {
        if (value == topology) {
            return name;
        }
    }
    return {};
}

[[nodiscard]] constexpr std::optional<MapTopology> map_topology_from_string(std::string_view name) noexcept
{
    for (const auto& [value, text] : MapTopologyNames) {
        if (text == name) {
            return value;
        }
    }
    return std::nullopt;
}

} // namespace asst::blackflow
