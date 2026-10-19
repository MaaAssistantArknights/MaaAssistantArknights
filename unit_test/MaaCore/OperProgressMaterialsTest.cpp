#include <catch2/catch_test_macros.hpp>

#include "Task/OperProgress/OperProgressMaterials.h"

using namespace asst::oper_progress;

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
    int remaining = 8;
    for (const int obtained : { 4, 2, 1, 1 }) {
        ChipPackBatch batch("3272", remaining);
        batch.observe("3212", remaining - obtained);
        REQUIRE(batch.remaining() == remaining);
        batch.observe("3272", obtained);
        // StageDrops.stats is cumulative; repeated callbacks must not count the same drop twice.
        batch.observe("3272", obtained);
        remaining = batch.remaining();
    }
    REQUIRE(remaining == 0);
}

TEST_CASE("A batch with only the paired chip leaves the full shortfall")
{
    ChipPackBatch batch("3272", 3);
    batch.observe("3212", 3);
    REQUIRE(batch.obtained() == 0);
    REQUIRE(batch.remaining() == 3);
    batch.observe("3272", -1);
    REQUIRE(batch.remaining() == 3);
    batch.observe("3272", 1);
    batch.observe("3272", 0);
    REQUIRE(batch.obtained() == 1);
    batch.observe("3272", 4);
    REQUIRE(batch.remaining() == 0);
}
