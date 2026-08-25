#include <catch2/catch_test_macros.hpp>

#include <meojson/json.hpp>

#include "Task/OperatorDevelopment/OperatorDevelopmentPlanParser.h"

namespace
{
std::optional<asst::OperatorDevelopmentPlan> parse(std::string_view text)
{
    const auto value = json::parse(text);
    REQUIRE(value.has_value());
    return asst::parse_operator_development_plan(*value);
}
} // namespace

TEST_CASE("Operator development plan preserves order and duplicates")
{
    const auto plan = parse(R"({"plans":[
        {"name":"A","elite":1},
        {"name":"A","skills":7},
        {"name":"A","skill":3,"skill_master":3},
        {"name":"A","elite":1}
    ]})");
    REQUIRE(plan.has_value());
    REQUIRE(plan->size() == 4);
    REQUIRE(plan->at(0).action == asst::OperatorDevelopmentAction::Elite);
    REQUIRE(plan->at(1).action == asst::OperatorDevelopmentAction::Skills);
    REQUIRE(plan->at(2).action == asst::OperatorDevelopmentAction::Mastery);
    REQUIRE(plan->at(3).action == asst::OperatorDevelopmentAction::Elite);
}

TEST_CASE("Operator development plan accepts boundaries and empty plan")
{
    REQUIRE(parse(R"({"plans":[]})")->empty());
    REQUIRE(parse(R"({"plans":[{"name":"A","elite":1},{"name":"A","elite":2}]})").has_value());
    REQUIRE(parse(R"({"plans":[{"name":"A","skills":2},{"name":"A","skills":7}]})").has_value());
    REQUIRE(parse(R"({"plans":[{"name":"A","skill":1,"skill_master":1},{"name":"A","skill":3,"skill_master":3}]})")
                .has_value());
}

TEST_CASE("Operator development plan rejects malformed targets")
{
    const std::vector<std::string_view> invalid {
        R"({})",
        R"({"plans":{}})",
        R"({"plans":[{"name":"","elite":1}]})",
        R"({"plans":[{"name":"   ","elite":1}]})",
        R"({"plans":[{"name":"A","elite":0}]})",
        R"({"plans":[{"name":"A","elite":1.5}]})",
        R"({"plans":[{"name":"A","elite":1e100}]})",
        R"({"plans":[{"name":"A","skills":8}]})",
        R"({"plans":[{"name":"A","skill":1}]})",
        R"({"plans":[{"name":"A","skill_master":1}]})",
        R"({"plans":[{"name":"A","elite":1,"skills":2}]})",
        R"({"plans":[{"name":"A","elite":1,"unknown":true}]})",
    };
    for (const auto value : invalid) {
        REQUIRE_FALSE(parse(value).has_value());
    }
}
