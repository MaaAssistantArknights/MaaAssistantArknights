#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "Utils/InfrastDronesUsage.hpp"

using asst::infrast::is_trade_drones_usage_mismatched;

TEST_CASE("Trade drone reminders require all orders to be the opposite type", "[infrast][drones]")
{
    const std::vector<std::string> money { "Money", "Money", "Money" };
    const std::vector<std::string> jade { "SyntheticJade", "SyntheticJade" };
    REQUIRE(is_trade_drones_usage_mismatched("SyntheticJade", money));
    REQUIRE(is_trade_drones_usage_mismatched("Money", jade));
    REQUIRE_FALSE(is_trade_drones_usage_mismatched("Money", money));
    REQUIRE_FALSE(is_trade_drones_usage_mismatched("SyntheticJade", jade));
}

TEST_CASE("Mixed, skipped and unrecognized trade stations do not trigger reminders", "[infrast][drones]")
{
    const std::vector<std::vector<std::string>> products {
        {}, { "" }, { "Money", "SyntheticJade" }, { "Money", "" }, { "", "SyntheticJade" }, { "Money", "Unknown" },
    };
    for (const auto& facilities : products) {
        REQUIRE_FALSE(is_trade_drones_usage_mismatched("Money", facilities));
        REQUIRE_FALSE(is_trade_drones_usage_mismatched("SyntheticJade", facilities));
    }
}

TEST_CASE("Non-trade and already used drone settings do not trigger reminders", "[infrast][drones]")
{
    const std::vector<std::string> money { "Money" };
    const std::vector<std::string> jade { "SyntheticJade" };
    for (const auto usage : { "_NotUse", "_Used", "CombatRecord", "PureGold", "OriginStone", "Chip", "", "Unknown" }) {
        INFO("usage=" << usage);
        REQUIRE_FALSE(is_trade_drones_usage_mismatched(usage, money));
        REQUIRE_FALSE(is_trade_drones_usage_mismatched(usage, jade));
    }
}
