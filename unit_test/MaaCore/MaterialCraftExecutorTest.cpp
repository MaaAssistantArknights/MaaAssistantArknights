#include <catch2/catch_test_macros.hpp>

#include "Utils/MaterialCraftExecutor.h"

#include <limits>

using namespace asst;

struct LiveCraftFixture
{
    std::vector<MaterialFormula> formulas {
        { "1", "A", 1, 0, 0, { { "base", 3 } } },
        { "2", "B", 1, 0, 0, { { "A", 2 }, { "base", 1 } } },
        { "3", "C", 1, 0, 0, { { "A", 1 }, { "B", 1 } } },
    };
    MaterialInventory live { { "base", 100 }, { "A", 0 }, { "B", 0 }, { "C", 0 } };
    std::vector<std::string> reads, crafts;
    bool stop = false, failed_read = false, byproduct = false, missing_slot = false, stalled = false;
    int batch_limit = 100;

    MaterialCraftExecutor executor()
    {
        return MaterialCraftExecutor(
            formulas,
            {
                [this](const MaterialFormula& f) -> std::optional<MaterialInventory> {
                    reads.push_back(f.item_id);
                    if (failed_read) {
                        return std::nullopt;
                    }
                    MaterialInventory result;
                    for (const auto& c : f.costs) {
                        result[c.item_id] = live[c.item_id];
                    }
                    if (missing_slot) {
                        result.erase(f.costs.front().item_id);
                    }
                    return result;
                },
                [this](const MaterialCraftOperation& op) -> std::optional<int> {
                    crafts.push_back(op.formula.item_id);
                    const int batches = std::min(op.batches, batch_limit);
                    if (!stalled) {
                        for (const auto& c : op.formula.costs) {
                            REQUIRE(live[c.item_id] >= c.count * batches);
                            live[c.item_id] -= c.count * batches;
                        }
                        live[op.formula.item_id] += batches * op.formula.count;
                        if (byproduct && op.formula.item_id == "A") {
                            ++live["B"];
                        }
                    }
                    return batches;
                },
                [this] { return stop; },
            });
    }
};

TEST_CASE("Recipe execution uses live stock and rechecks shared ingredients")
{
    LiveCraftFixture f;
    auto e = f.executor();
    REQUIRE(e.craft({ "C", 1 }));
    REQUIRE(f.live["C"] == 1);
    REQUIRE(f.live["A"] == 0);
    REQUIRE(f.live["B"] == 0);
    REQUIRE(f.live["base"] == 90);
    REQUIRE(f.reads.front() == "C");
    REQUIRE(f.reads.back() == "C");
    REQUIRE(f.crafts.back() == "C");
}

TEST_CASE("Sufficient recipe stock skips recursion and targets remain additive")
{
    LiveCraftFixture f;
    f.live["A"] = 20;
    f.live["B"] = 20;
    auto e = f.executor();
    REQUIRE(e.craft({ "C", 2 }));
    REQUIRE(f.crafts == std::vector<std::string> { "C" });
    REQUIRE(e.craft({ "A", 1 }));
    REQUIRE(f.live["A"] == 19);
}

TEST_CASE("Useful byproducts are observed before scheduling another child")
{
    LiveCraftFixture f;
    f.byproduct = true;
    auto e = f.executor();
    REQUIRE(e.craft({ "C", 1 }));
    REQUIRE(f.crafts == std::vector<std::string> { "A", "C" });
}

TEST_CASE("Every limited batch rereads its recipe")
{
    LiveCraftFixture f;
    f.batch_limit = 1;
    auto e = f.executor();
    REQUIRE(e.craft({ "A", 3 }));
    REQUIRE(f.crafts.size() == 3);
    REQUIRE(f.reads.size() == 3);
}

TEST_CASE("Unreadable incomplete or unavailable ingredients cannot start crafting")
{
    LiveCraftFixture f;
    SECTION("OCR failed")
    {
        f.failed_read = true;
    }
    SECTION("One slot absent")
    {
        f.missing_slot = true;
    }
    SECTION("Base material missing")
    {
        f.live["base"] = 0;
    }
    SECTION("Cancelled")
    {
        f.stop = true;
    }
    auto e = f.executor();
    REQUIRE_FALSE(e.craft({ "A", 1 }));
    REQUIRE(f.crafts.empty());
}

TEST_CASE("Cyclic overflowing and unconfirmed recipes terminate")
{
    LiveCraftFixture f;
    SECTION("Cycle")
    {
        f.formulas.front().costs = { { "C", 1 } };
        auto e = f.executor();
        REQUIRE_FALSE(e.craft({ "C", 1 }));
    }
    SECTION("Quantity overflow")
    {
        auto e = f.executor();
        REQUIRE_FALSE(e.craft({ "A", std::numeric_limits<int>::max() }));
        REQUIRE(f.crafts.empty());
    }
    SECTION("Child output not reflected on parent")
    {
        f.stalled = true;
        auto e = f.executor();
        REQUIRE_FALSE(e.craft({ "B", 1 }));
        REQUIRE(f.crafts.size() == 1);
    }
}
