#include <catch2/catch_test_macros.hpp>

#include "Common/InfrastSimpleMode.h"

TEST_CASE("Simple base mode can collect rewards without entering facilities", "[infrast][simple]")
{
    REQUIRE(asst::infrast::build_simple_facilities("_NotUse", false).empty());
}

TEST_CASE("Simple base mode selects facilities by drone usage", "[infrast][simple]")
{
    for (const auto drones : { "Money", "SyntheticJade" }) {
        INFO(drones);
        REQUIRE(asst::infrast::build_simple_facilities(drones, false) == std::vector<std::string> { "Trade" });
    }
    for (const auto drones : { "CombatRecord", "PureGold", "OriginStone", "Chip" }) {
        INFO(drones);
        REQUIRE(asst::infrast::build_simple_facilities(drones, false) == std::vector<std::string> { "Mfg" });
    }
}

TEST_CASE("Simple base mode processes reception independently of drones", "[infrast][simple]")
{
    REQUIRE(asst::infrast::build_simple_facilities("_NotUse", true) == std::vector<std::string> { "Reception" });
    REQUIRE(asst::infrast::build_simple_facilities("Money", true) == std::vector<std::string> { "Trade", "Reception" });
    REQUIRE(
        asst::infrast::build_simple_facilities("PureGold", true) == std::vector<std::string> { "Mfg", "Reception" });
}

TEST_CASE("Simple base mode ignores unsupported drone usage", "[infrast][simple]")
{
    for (const auto drones : { "", "_Used", "Unknown" }) {
        INFO(drones);
        REQUIRE(asst::infrast::build_simple_facilities(drones, false).empty());
        REQUIRE(asst::infrast::build_simple_facilities(drones, true) == std::vector<std::string> { "Reception" });
    }
}
