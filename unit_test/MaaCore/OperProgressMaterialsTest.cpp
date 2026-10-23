#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Task/OperProgress/OperProgressMaterials.h"

using namespace asst::oper_progress;

TEST_CASE("Factory restoration only matches the recorded device and room")
{
    const FactoryProductionState state { "device-1", "B201", "PureGold" };
    REQUIRE(state.matches("device-1", "B201"));
    REQUIRE_FALSE(state.matches("device-2", "B201"));
    REQUIRE_FALSE(state.matches("device-1", "B302"));
    REQUIRE_FALSE(state.matches("", "B201"));
    REQUIRE_FALSE(state.matches("device-1", ""));

    const FactoryProductionState missing_device { "", "B201", "PureGold" };
    REQUIRE_FALSE(missing_device.matches("", "B201"));
    const FactoryProductionState missing_room { "device-1", "", "PureGold" };
    REQUIRE_FALSE(missing_room.matches("device-1", ""));
}

TEST_CASE("Factory room recognition rejects incomplete and ambiguous identifiers")
{
    REQUIRE(parse_factory_room("B201") == "B201");
    REQUIRE(parse_factory_room("B302") == "B302");
    REQUIRE(parse_factory_room("制造站 B201") == "B201");
    REQUIRE(parse_factory_room(" B302 MANUFACTURE ") == "B302");
    for (const auto* text : { "",
                              "制造站",
                              "B20",
                              "B21",
                              "B2O1",
                              "B001",
                              "B401",
                              "B200",
                              "B204",
                              "B2010",
                              "B3029",
                              "B201 B302",
                              "B201B201" }) {
        INFO(text);
        REQUIRE_FALSE(parse_factory_room(text));
    }
}

TEST_CASE("Every failed production step still restores the factory")
{
    constexpr const char* steps[] = { "select product", "set quantity", "confirm production", "collect chips" };
    for (int failed_step = 0; failed_step < 4; ++failed_step) {
        INFO("Failing step: " << steps[failed_step]);
        std::string actions;
        int attempted_steps = 0;
        const bool result = manufacture_with_restore(
            [&] {
                actions += 'P';
                for (int step = 0; step < 4; ++step) {
                    ++attempted_steps;
                    if (step == failed_step) {
                        return false;
                    }
                }
                return true;
            },
            [&] {
                actions += 'R';
                return true;
            },
            [] { return false; });
        REQUIRE_FALSE(result);
        REQUIRE(attempted_steps == failed_step + 1);
        REQUIRE(actions == "PR");
    }
}

TEST_CASE("Successful manufacturing requires successful restoration")
{
    for (const bool restored : { true, false }) {
        INFO("Restoration result: " << restored);
        std::string actions;
        const bool result = manufacture_with_restore(
            [&] {
                actions += 'P';
                return true;
            },
            [&] {
                actions += 'R';
                return restored;
            },
            [] { return false; });
        REQUIRE(result == restored);
        REQUIRE(actions == "PR");
    }
}

TEST_CASE("Cancellation prevents later factory actions and successful completion")
{
    SECTION("Stopped before production performs no actions")
    {
        std::string actions;
        const bool result = manufacture_with_restore(
            [&] {
                actions += 'P';
                return true;
            },
            [&] {
                actions += 'R';
                return true;
            },
            [] { return true; });
        REQUIRE_FALSE(result);
        REQUIRE(actions.empty());
    }

    SECTION("Stopped during production never starts restoration")
    {
        for (const bool produced : { true, false }) {
            INFO("Production result: " << produced);
            bool cancelled = false;
            std::string actions;
            const bool result = manufacture_with_restore(
                [&] {
                    actions += 'P';
                    cancelled = true;
                    return produced;
                },
                [&] {
                    actions += 'R';
                    return true;
                },
                [&] { return cancelled; });
            REQUIRE_FALSE(result);
            REQUIRE(actions == "P");
        }
    }

    SECTION("Stopped during restoration cannot report success")
    {
        bool cancelled = false;
        std::string actions;
        const bool result = manufacture_with_restore(
            [&] {
                actions += 'P';
                return true;
            },
            [&] {
                actions += 'R';
                cancelled = true;
                return true;
            },
            [&] { return cancelled; });
        REQUIRE_FALSE(result);
        REQUIRE(actions == "PR");
    }
}

TEST_CASE("Dual chip requirements use the promotion shortfall")
{
    const auto empty = parse_dual_chip_requirements("0/4");
    REQUIRE(empty);
    REQUIRE(empty->missing() == 4);
    REQUIRE(empty->chip_packs() == 8);
    REQUIRE(empty->catalysts() == 4);

    const auto partial = parse_dual_chip_requirements(" 2 / 4 ");
    REQUIRE(partial);
    REQUIRE(partial->chip_packs() == 4);
    REQUIRE(partial->catalysts() == 2);

    const auto five_star = parse_dual_chip_requirements("1/3");
    REQUIRE(five_star);
    REQUIRE(five_star->chip_packs() == 4);
    REQUIRE(five_star->catalysts() == 2);

    const auto complete = parse_dual_chip_requirements("12/4");
    REQUIRE(complete);
    REQUIRE(complete->chip_packs() == 0);
    REQUIRE(complete->catalysts() == 0);
}

TEST_CASE("Uncertain inventory never becomes a zero inventory")
{
    for (const auto* text : { "", "4", "0/", "/4", "O/4", "0/0", "0/40", "0/4/1", "-1/4", "0/4x" }) {
        INFO(text);
        REQUIRE_FALSE(parse_dual_chip_requirements(text));
    }
    REQUIRE_FALSE(parse_material_number("2147483648"));
    REQUIRE_FALSE(parse_material_number("4/1"));
    REQUIRE_FALSE(parse_material_number("4 stock"));
    REQUIRE(parse_material_number("0") == 0);
    REQUIRE(parse_material_count("8/2")->owned == 8);
    REQUIRE(parse_material_count("4/1")->owned == 4);
}

TEST_CASE("Chip farming repeats only the actual target shortfall")
{
    struct ExpectedBatch
    {
        int required;
        int obtained;
        int remaining;
    };

    constexpr ExpectedBatch batches[] = { { 8, 4, 4 }, { 4, 2, 2 }, { 2, 1, 1 }, { 1, 1, 0 } };
    int remaining = 8;
    for (const auto& expected : batches) {
        INFO("Batch requires " << expected.required << " chips and obtains " << expected.obtained);
        REQUIRE(remaining == expected.required);
        ChipPackBatch batch("3272", remaining);
        REQUIRE(batch.obtained() == 0);
        REQUIRE(batch.remaining() == expected.required);

        batch.observe("3212", expected.required - expected.obtained);
        REQUIRE(batch.obtained() == 0);
        REQUIRE(batch.remaining() == expected.required);
        for (int cumulative = 1; cumulative <= expected.obtained; ++cumulative) {
            INFO("Cumulative target drops: " << cumulative);
            batch.observe("3272", cumulative);
            REQUIRE(batch.obtained() == cumulative);
            REQUIRE(batch.remaining() == expected.required - cumulative);

            // StageDrops.stats is cumulative; repeated callbacks must not count the same drop twice.
            batch.observe("3272", cumulative);
            REQUIRE(batch.obtained() == cumulative);
            REQUIRE(batch.remaining() == expected.required - cumulative);

            batch.observe("3272", cumulative - 1);
            REQUIRE(batch.obtained() == cumulative);
            REQUIRE(batch.remaining() == expected.required - cumulative);
        }
        REQUIRE(batch.obtained() == expected.obtained);
        REQUIRE(batch.remaining() == expected.remaining);
        remaining = batch.remaining();
        REQUIRE(remaining == expected.remaining);
    }
    REQUIRE(remaining == 0);
}

TEST_CASE("A batch with only the paired chip leaves the full shortfall")
{
    ChipPackBatch batch("3272", 3);
    for (const int cumulative : { 1, 2, 3, 3, 2, 4 }) {
        INFO("Cumulative paired chip drops: " << cumulative);
        batch.observe("3212", cumulative);
        REQUIRE(batch.obtained() == 0);
        REQUIRE(batch.remaining() == 3);
    }
}

TEST_CASE("Chip batches reject stale counts and cap credited drops at the shortfall")
{
    struct ExpectedObservation
    {
        int cumulative;
        int obtained;
        int remaining;
    };

    constexpr ExpectedObservation observations[] = {
        { -1, 0, 3 }, { 0, 0, 3 },  { 2, 2, 1 }, { 2, 2, 1 }, { 1, 2, 1 },
        { 0, 2, 1 },  { -1, 2, 1 }, { 5, 3, 0 }, { 5, 3, 0 }, { 1, 3, 0 },
    };
    ChipPackBatch batch("3272", 3);
    for (const auto& expected : observations) {
        INFO("Cumulative target drops: " << expected.cumulative);
        batch.observe("3272", expected.cumulative);
        REQUIRE(batch.obtained() == expected.obtained);
        REQUIRE(batch.remaining() == expected.remaining);
    }
}
