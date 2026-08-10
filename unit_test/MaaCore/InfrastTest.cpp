#include <catch2/catch_test_macros.hpp>

#include "Common/InfrastData.h"
#include "Task/Infrast/InfrastScore.h"

using asst::infrast::ScoreContext;
using asst::infrast::ScoreOper;
using asst::infrast::select_best_opers;
using asst::infrast::should_short_circuit_mfg;

TEST_CASE("infrast skill identity candidates intersect", "[infrast]")
{
    const auto candidates = asst::infrast::intersect_operator_ids({ { "char_a", "char_b" }, { "char_b", "char_c" } });

    REQUIRE(candidates.size() == 1);
    CHECK(candidates.contains("char_b"));
}

TEST_CASE("infrast task data commits selected operators", "[infrast]")
{
    asst::infrast::OperatorSelection data;
    data.pending_operator_ids = { "char_a", "char_b" };

    data.commit_pending();

    CHECK(data.operator_ids.contains("char_a"));
    CHECK(data.operator_ids.contains("char_b"));
    CHECK(data.pending_operator_ids.empty());

    data.pending_operator_ids = { "char_c" };
    data.discard_pending();
    CHECK(data.operator_ids == asst::infrast::OperatorIds { "char_a", "char_b" });
    CHECK(data.pending_operator_ids.empty());
}

TEST_CASE("infrast production scores combinations", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Mfg";
    context.product = "PureGold";
    context.level = 2;
    context.slots = 2;
    context.mood_threshold = 0.5;

    const std::vector<ScoreOper> opers = {
        { { "bskill_man_spd_variable31" }, "", "bubble", 0.5 },
        { { "bskill_man_spd&limit&cost2" }, "", "storage", 1.0 },
        { { "bskill_man_spd2" }, "", "speed", 1.0 },
        { { "bskill_man_spd3" }, "", "tired", 0.49 },
    };

    const auto result = select_best_opers(opers, context);

    CHECK(result.indices == std::vector<size_t> { 0, 1 });
}

TEST_CASE("infrast single slot facilities use migrated priorities", "[infrast][score]")
{
    const std::vector<ScoreOper> opers = {
        { { "bskill_hire_spd2" }, "", "ordinary", 1.0 },
        { { "bskill_hire_spd5" }, "", "fast", 1.0 },
    };
    ScoreContext context;
    context.facility = "Office";

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });

    context.facility = "Power";
    const std::vector<ScoreOper> power_opers = {
        { { "bskill_pow_spd1" }, "", "basic", 1.0 },
        { { "bskill_power_rec_spd&addition2" }, "", "addition", 1.0 },
    };
    CHECK(select_best_opers(power_opers, context).indices == std::vector<size_t> { 1 });
}

TEST_CASE("infrast reception excludes disabled skill and vigil", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Reception";
    context.slots = 2;
    const std::vector<ScoreOper> opers = {
        { { "bskill_meet_spdowned1" }, "", "disabled", 1.0 },
        { { "bskill_meet_spd3" }, "char_427_vigil", "vigil", 1.0 },
        { { "bskill_meet_spd&cost" }, "", "priority", 1.0 },
        { { "bskill_meet_spdnotowned2" }, "", "second", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 2, 3 });
}

TEST_CASE("infrast dorm reacts only to committed stable identities", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Dorm";
    context.slots = 2;
    context.selected_operator_ids = { "char_391_rosmon" };
    const std::vector<ScoreOper> opers = {
        { { "bskill_dorm_all3" }, "", "generic", 1.0 },
        { { "bskill_dorm_all&bd_n1_2", "bskill_dorm_all&bd_n1_n3" }, "", "czerny", 1.0 },
        { { "bskill_dorm_bdnum" }, "", "special", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1, 2 });

    context.selected_operator_ids.clear();
    CHECK(select_best_opers(opers, context).indices != std::vector<size_t> { 1, 2 });
}

TEST_CASE("infrast manufacturing short circuit requires every guard", "[infrast][short-circuit]")
{
    CHECK_FALSE(should_short_circuit_mfg(false, false, 3, 3, 1.2, 0.38));
    CHECK_FALSE(should_short_circuit_mfg(true, true, 3, 3, 1.2, 0.38));
    CHECK_FALSE(should_short_circuit_mfg(true, false, 2, 3, 1.2, 0.38));
    CHECK_FALSE(should_short_circuit_mfg(true, false, 3, 3, std::nullopt, 0.38));
    CHECK_FALSE(should_short_circuit_mfg(true, false, 3, 3, 1.14, 0.38));
    CHECK(should_short_circuit_mfg(true, false, 3, 3, 1.15, 0.38));
}

TEST_CASE("infrast facility plan preserves each mode contract", "[infrast][plan]")
{
    using asst::infrast::build_facility_plan;
    using asst::infrast::FacilityPlanMode;
    using asst::infrast::FacilityStep;

    const std::vector<std::string> unordered_default = {
        "Training", "Trade", "Dorm", "Trade", "Control", "Processing"
    };
    const auto default_plan = build_facility_plan(FacilityPlanMode::Default, unordered_default);
    REQUIRE(default_plan);
    CHECK(
        *default_plan == std::vector {
                             FacilityStep::MfgInspect,
                             FacilityStep::DormPrepare,
                             FacilityStep::ControlForce,
                             FacilityStep::Trade,
                             FacilityStep::ControlVacancy,
                             FacilityStep::DormFill,
                             FacilityStep::Processing,
                             FacilityStep::Training,
                         });

    const std::vector<std::string> ordered = { "Trade", "Mfg", "Dorm", "Reception" };
    const auto custom_plan = build_facility_plan(FacilityPlanMode::Custom, ordered);
    REQUIRE(custom_plan);
    CHECK(
        *custom_plan == std::vector {
                            FacilityStep::Trade,
                            FacilityStep::Mfg,
                            FacilityStep::DormPrepare,
                            FacilityStep::Reception,
                        });

    const auto rotation_plan = build_facility_plan(FacilityPlanMode::Rotation, ordered);
    REQUIRE(rotation_plan);
    CHECK(*rotation_plan == std::vector { FacilityStep::Trade, FacilityStep::Mfg, FacilityStep::Reception });
    CHECK_FALSE(build_facility_plan(FacilityPlanMode::Default, { "Unknown" }));
}

TEST_CASE("infrast manufacturing respects product slots and mood boundary", "[infrast][score]")
{
    const std::vector<ScoreOper> opers = {
        { { "bskill_man_exp3" }, "", "record", 0.5 },
        { { "bskill_man_gold2" }, "", "gold", 1.0 },
        { { "bskill_man_spd3" }, "", "generic", 1.0 },
        { { "bskill_man_spd2" }, "", "below-threshold", 0.499 },
    };

    ScoreContext context;
    context.facility = "Mfg";
    context.level = 3;
    context.mood_threshold = 0.5;

    context.product = "CombatRecord";
    context.slots = 1;
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0 });

    context.product = "PureGold";
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });

    for (const int slots : { 1, 2, 3 }) {
        context.slots = slots;
        const auto result = select_best_opers(opers, context);
        CHECK(result.indices.size() == static_cast<size_t>(slots));
        CHECK(std::ranges::find(result.indices, 3) == result.indices.end());
    }
}

TEST_CASE("infrast manufacturing can select a complete abyssal roster across rooms", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Mfg";
    context.product = "PureGold";
    context.level = 3;
    context.slots = 3;
    context.use_abyssal_hunter = true;
    context.selected_operator_ids = { "char_474_glady" };

    const std::vector<ScoreOper> opers = {
        { { "bskill_man_spd3" }, "", "generic-a", 1.0 }, { { "bskill_man_spd3" }, "", "generic-b", 1.0 },
        { { "bskill_man_spd3" }, "", "generic-c", 1.0 }, { { }, "char_263_skadi", "skadi", 1.0 },
        { { }, "char_143_ghost", "specter", 1.0 },       { { }, "char_4145_ulpia", "ulpianus", 1.0 },
        { { }, "char_218_cuttle", "andreana", 1.0 },
    };

    const auto result = select_best_opers(opers, context);
    size_t abyssal_count = 0;
    for (const size_t index : result.indices) {
        if (index >= 3) {
            ++abyssal_count;
        }
    }
    CHECK(abyssal_count == 2);
}

TEST_CASE("infrast trade prefers complete paired skills", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Trade";
    context.product = "Money";
    context.level = 3;
    context.slots = 2;

    const std::vector<ScoreOper> opers = {
        { { "bskill_tra_lappland2" }, "", "lappland", 1.0 },
        { { "bskill_tra_texas2" }, "", "texas", 1.0 },
        { { "bskill_tra_spd2" }, "", "speed-a", 1.0 },
        { { "bskill_tra_spd2" }, "", "speed-b", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0, 1 });
}

TEST_CASE("infrast stable identity gates power linkage", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Power";
    context.selected_operator_ids = { "char_4000_jnight" };

    std::vector<ScoreOper> opers = {
        { { "bskill_pow_spd1" }, "char_285_medic2", "stable-lancet", 1.0 },
        { { "bskill_power_rec_spd&addition2" }, "", "ordinary", 1.0 },
    };
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0 });

    opers[0].operator_id.clear();
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });
}

TEST_CASE("infrast control keeps required operator pairs together", "[infrast][score]")
{
    ScoreContext context;
    context.facility = "Control";
    context.slots = 2;
    const std::vector<ScoreOper> opers = {
        { { "bskill_ctrl_token_p_spd2", "bskill_ctrl_cost_felyne" }, "", "yato", 1.0 },
        { { "bskill_ctrl_token_t_spd" }, "", "noir", 1.0 },
        { { "bskill_ctrl_sp" }, "", "generic-a", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-b", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0, 1 });
}
