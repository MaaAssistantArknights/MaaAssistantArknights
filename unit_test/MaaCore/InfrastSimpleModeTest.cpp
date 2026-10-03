#include <catch2/catch_test_macros.hpp>

#include "Common/InfrastSimpleMode.h"
#include "Common/MainScreenTask.h"

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

TEST_CASE("Prefixed base navigation retains PC main screen recognition", "[infrast][pc]")
{
    const std::unordered_set<std::string> themed_entries;
    REQUIRE(asst::is_main_screen_task("Infrast", themed_entries));
    REQUIRE(asst::is_main_screen_task("InfrastSimple@Infrast", themed_entries));
    REQUIRE(asst::is_main_screen_task("Daily@InfrastSimple@Infrast", themed_entries));
}

TEST_CASE("Prefixed theme entries retain PC main screen recognition", "[infrast][pc]")
{
    const std::unordered_set<std::string> themed_entries { "Infrast@MainTheme_Rhodes@Entry" };
    REQUIRE(asst::is_main_screen_task("Infrast@MainTheme_Rhodes@Entry", themed_entries));
    REQUIRE(asst::is_main_screen_task("InfrastSimple@Infrast@MainTheme_Rhodes@Entry", themed_entries));
    REQUIRE(asst::is_main_screen_task("Daily@InfrastSimple@Infrast@MainTheme_Rhodes@Entry", themed_entries));
}

TEST_CASE("Base notifications and rooms do not enable PC main screen recognition", "[infrast][pc]")
{
    const std::unordered_set<std::string> themed_entries { "Infrast@MainTheme_Rhodes@Entry" };
    for (const auto name :
         { "InfrastSimple@InfrastEnteredFlag", "InfrastSimple@InfrastNotification", "Dorm@Entry", "" }) {
        INFO(name);
        REQUIRE_FALSE(asst::is_main_screen_task(name, themed_entries));
    }
}
