#include <catch2/catch_test_macros.hpp>

#include "Utils/RoguelikeEncounterOptionSelector.h"

namespace selector = asst::RoguelikeEncounterOptionSelector;

TEST_CASE("Duplicate option occurrence selects the requested enabled option")
{
    const std::vector<selector::VisibleOption> options {
        { true, "我的了！" },
        { true, "我的了！" },
    };
    const selector::Rule rule {
        .preferred_options = { { { "我的了！" }, 2 } },
    };

    const auto result = selector::select(options, rule, {});
    REQUIRE(result.index == 1);
}

TEST_CASE("Disabled preferred option is skipped")
{
    const std::vector<selector::VisibleOption> options {
        { false, "离开" },
        { true, "不关我的事" },
    };
    const selector::Rule rule {
        .preferred_options = { { { "离开" }, 1 } },
        .safe_fallback_options = { "不关我的事" },
    };

    const auto result = selector::select(options, rule, {});
    REQUIRE(result.index == 1);
    REQUIRE(result.used_safe_fallback);
}

TEST_CASE("Variant matching tolerates a changed visible option count")
{
    const std::vector<selector::VisibleOption> options {
        { true, "甲" },
        { true, "乙" },
    };
    const selector::Rule base {
        .preferred_options = { { { "甲" }, 1 } },
    };
    const std::vector<selector::Rule> variants {
        { .id = "same-title-stage", .option_texts = { "甲", "乙", "丙" }, .preferred_options = { { { "乙" }, 1 } } },
    };

    const auto result = selector::select(options, base, variants);
    REQUIRE(result.index == 1);
    REQUIRE(result.matched_variant);
    REQUIRE(result.rule_id == "same-title-stage");
}

TEST_CASE("No safe option does not fall through to blind selection")
{
    const std::vector<selector::VisibleOption> options {
        { true, "遭遇战斗" },
    };
    const selector::Rule rule;

    const auto result = selector::select(options, rule, {});
    REQUIRE_FALSE(result.index.has_value());
}
