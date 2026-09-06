#include <catch2/catch_test_macros.hpp>

#include <limits>

#include "Utils/MaterialCraftPlanner.h"

using namespace asst;

namespace
{
MaterialFormula recipe(std::string id, std::string product, std::vector<MaterialAmount> costs, int output = 1)
{
    return { std::move(id), std::move(product), output, 100, 360000, std::move(costs) };
}
}

TEST_CASE("Craft dependencies before their consumer and reuse rounded batch leftovers")
{
    MaterialCraftPlanner planner({ recipe("1", "A", { { "ore", 3 } }, 2), recipe("2", "B", { { "A", 3 } }) });
    const MaterialCraftRequest request { { { "B", 1 }, { "B", 1 } }, { { "ore", 9 } } };
    const auto plan = planner.build(request);
    REQUIRE(plan.valid);
    REQUIRE(plan.missing.empty());
    REQUIRE(plan.inventory.at("B") == 2);
    REQUIRE(plan.inventory.at("ore") == 0);
    REQUIRE(plan.inventory.at("A") == 0);
    REQUIRE(plan.operations.size() == 4);
    CHECK(plan.operations[0].formula.item_id == "A");
    CHECK(plan.operations[0].batches == 2);
    CHECK(plan.operations[1].formula.item_id == "B");
    CHECK(plan.operations[2].batches == 1);
    CHECK(request.inventory.at("ore") == 9);
}

TEST_CASE("Keep missing base ingredients explicit in an otherwise executable plan")
{
    MaterialCraftPlanner planner({ recipe("1", "A", { { "ore", 3 }, { "salt", 2 } }) });
    const auto plan = planner.build({ { { "A", 2 } }, { { "ore", 1 } } });
    REQUIRE(plan.valid);
    CHECK(plan.missing.at("ore") == 5);
    CHECK(plan.missing.at("salt") == 4);
    CHECK(plan.operations.front().batches == 2);
    CHECK(plan.inventory.at("ore") == 0);
}

TEST_CASE("Try alternative recipes without leaking inventory changes or shortages")
{
    MaterialCraftPlanner planner({ recipe("1", "A", { { "ore", 3 } }), recipe("2", "A", { { "salt", 2 } }) });
    const auto plan = planner.build({ { { "A", 1 } }, { { "ore", 1 }, { "salt", 2 } } });
    REQUIRE(plan.valid);
    REQUIRE(plan.missing.empty());
    CHECK(plan.inventory.at("ore") == 1);
    CHECK(plan.inventory.at("salt") == 0);
    CHECK(plan.operations.front().formula.formula_id == "2");
}

TEST_CASE("Reject a cyclic recipe but allow a noncyclic alternative")
{
    const auto cycle = recipe("1", "A", { { "B", 1 } });
    const auto back = recipe("2", "B", { { "A", 1 } });
    REQUIRE_FALSE(MaterialCraftPlanner({ cycle, back }).build({ { { "A", 1 } }, {} }).valid);
    const auto plan = MaterialCraftPlanner({ cycle, back, recipe("3", "A", { { "ore", 1 } }) })
                          .build({ { { "A", 1 } }, { { "ore", 1 } } });
    REQUIRE(plan.valid);
    CHECK(plan.operations.size() == 1);
    CHECK(plan.operations.front().formula.formula_id == "3");
}

TEST_CASE("Accumulate craft cost with 64 bit multiplication")
{
    auto formula = recipe("1", "A", { { "ore", 1 } });
    formula.ap_cost = 2'880'000;
    const auto plan = MaterialCraftPlanner({ formula }).build({ { { "A", 1000 } }, { { "ore", 1000 } } });
    REQUIRE(plan.valid);
    CHECK(plan.ap_cost == 2'880'000'000LL);
    CHECK(plan.gold_cost == 100000);
}

TEST_CASE("Reject quantity overflow before producing an executable plan")
{
    MaterialCraftPlanner planner({ recipe("1", "A", { { "ore", 3 } }) });
    const auto plan = planner.build({ { { "A", std::numeric_limits<int>::max() } }, {} });
    REQUIRE_FALSE(plan.valid);
    REQUIRE_FALSE(plan.error.empty());
}

TEST_CASE("A plan owns its recipes after the planner is destroyed")
{
    const auto plan =
        MaterialCraftPlanner({ recipe("1", "A", { { "ore", 1 } }) }).build({ { { "A", 1 } }, { { "ore", 1 } } });
    REQUIRE(plan.valid);
    CHECK(plan.operations.front().formula.costs.front().item_id == "ore");
}

TEST_CASE("Cancellation and invalid targets never yield a valid plan")
{
    MaterialCraftPlanner planner({ recipe("1", "A", { { "ore", 1 } }) });
    const auto cancelled = planner.build({ { { "A", 1 } }, {} }, [] { return true; });
    CHECK_FALSE(cancelled.valid);
    CHECK(cancelled.operations.empty());
    CHECK_FALSE(planner.build({ { { "A", 0 } }, {} }).valid);
    CHECK_FALSE(planner.build({ { { "unknown", 1 } }, {} }).valid);
    CHECK_FALSE(planner.build({ {}, {} }).valid);
}

TEST_CASE("Account for known currency without treating unknown currency as zero")
{
    MaterialCraftPlanner planner({ recipe("1", "A", { { "ore", 1 } }) });
    const auto known = planner.build({ { { "A", 2 } }, { { "ore", 2 }, { "4001", 150 } } });
    REQUIRE(known.valid);
    CHECK(known.inventory.at("4001") == 0);
    CHECK(known.missing.at("4001") == 50);
    const auto unknown = planner.build({ { { "A", 2 } }, { { "ore", 2 } } });
    REQUIRE(unknown.valid);
    CHECK_FALSE(unknown.inventory.contains("4001"));
    CHECK(unknown.missing.empty());
}
