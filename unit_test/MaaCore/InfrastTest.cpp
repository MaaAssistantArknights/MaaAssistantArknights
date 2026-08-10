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

TEST_CASE("infrast recognized identity must agree with skill candidates", "[infrast][identity]")
{
    using asst::infrast::operator_id_matches_candidates;

    CHECK(operator_id_matches_candidates({ }, "char_a"));
    CHECK(operator_id_matches_candidates({ "char_a", "char_b" }, "char_b"));
    CHECK_FALSE(operator_id_matches_candidates({ "char_a", "char_b" }, "char_c"));
    CHECK_FALSE(operator_id_matches_candidates({ }, ""));
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
    context.use_perception_information = true;
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

TEST_CASE("infrast control prefers swire and lungmen guard pair", "[infrast][score][identity]")
{
    ScoreContext context;
    context.facility = "Control";
    context.slots = 5;
    context.use_pinus_sylvestris = true;
    context.use_perception_information = true;
    context.selected_operator_ids = { "char_436_whispr" };
    const std::vector<ScoreOper> opers = {
        { { "bskill_ctrl_t_spd" }, "char_308_swire", "swire", 1.0 },
        { { "bskill_token_prod_spd3_lungmenguard" }, "char_1044_hsgma2", "hoshiguma", 1.0 },
        { { "bskill_ctrl_token_p_spd2", "bskill_ctrl_cost_felyne" }, "char_1029_yato2", "yato", 1.0 },
        { { "bskill_ctrl_token_t_spd" }, "char_1030_noirc2", "noir", 1.0 },
        { { "bskill_ctrl_psk" }, "char_420_flamtl", "flametail", 1.0 },
        { { "bskill_ctrl_fraction_knight" }, "char_4098_vvana", "viviana", 1.0 },
        { { "bskill_ctrl_cost_bd1", "bskill_ctrl_cost_bd2" }, "char_2015_dusk", "dusk", 1.0 },
        { { "bskill_ctrl_t_spd" }, "char_4071_peper", "paprika", 1.0 },
    };

    const auto result = select_best_opers(opers, context);
    REQUIRE(result.indices.size() == 5);
    CHECK(result.indices[0] == 0);
    CHECK(result.indices[1] == 1);
    CHECK(std::ranges::find(result.indices, 2) == result.indices.end());
    CHECK(std::ranges::find(result.indices, 3) == result.indices.end());
    CHECK(std::ranges::find(result.indices, 4) != result.indices.end());
    CHECK(std::ranges::find(result.indices, 5) != result.indices.end());
    CHECK(std::ranges::find(result.indices, 6) != result.indices.end());

    auto ambiguous_opers = opers;
    ambiguous_opers[0].operator_id.clear();
    const auto fallback = select_best_opers(ambiguous_opers, context);
    REQUIRE(fallback.indices.size() == 5);
    CHECK(fallback.indices[0] == 2);
    CHECK(fallback.indices[1] == 3);
    CHECK(std::ranges::find(fallback.indices, 0) == fallback.indices.end());

    auto oversized_context = context;
    oversized_context.slots = 7;
    CHECK(select_best_opers(opers, oversized_context).indices.size() == 5);
}

TEST_CASE("infrast pinus sylvestris team is opt in", "[infrast][score][options]")
{
    ScoreContext context;
    context.facility = "Control";
    context.slots = 5;
    const std::vector<ScoreOper> opers = {
        { { "bskill_ctrl_psk" }, "char_420_flamtl", "flametail", 1.0 },
        { { "bskill_ctrl_fraction_knight" }, "char_4098_vvana", "viviana", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-a", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-b", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-c", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-d", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-e", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 2, 3, 4, 5, 6 });

    context.use_pinus_sylvestris = true;
    const auto enabled = select_best_opers(opers, context);
    CHECK(std::ranges::find(enabled.indices, 0) != enabled.indices.end());
    CHECK(std::ranges::find(enabled.indices, 1) != enabled.indices.end());
}

TEST_CASE("infrast perception information team is opt in", "[infrast][score][options]")
{
    ScoreContext context;
    context.facility = "Office";
    context.slots = 1;
    const std::vector<ScoreOper> opers = {
        { { "bskill_hire_spd_memento" }, "char_436_whispr", "whisperain", 1.0 },
        { { "bskill_hire_spd2" }, "", "ordinary", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });

    context.use_perception_information = true;
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0 });
}

TEST_CASE("infrast perception information bonuses are opt in", "[infrast][score][options]")
{
    ScoreContext context;
    context.facility = "Mfg";
    context.product = "CombatRecord";
    context.level = 3;
    context.slots = 1;
    context.selected_operator_ids = { "char_436_whispr" };
    const std::vector<ScoreOper> opers = {
        { { "bskill_man_spd_bd2" }, "char_391_rosmon", "rosmontis", 1.0 },
        { { "bskill_man_spd3" }, "", "ordinary", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });

    context.use_perception_information = true;
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0 });
}

TEST_CASE("infrast worldly plight team is opt in", "[infrast][score][options]")
{
    ScoreContext context;
    context.facility = "Office";
    context.slots = 1;
    const std::vector<ScoreOper> opers = {
        { { "bskill_hire_spd_bd_n2" }, "char_473_mberry", "mulberry", 1.0 },
        { { "bskill_hire_spd2" }, "", "ordinary", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });

    context.use_worldly_plight = true;
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0 });
}

TEST_CASE("infrast worldly plight bonuses are opt in", "[infrast][score][options]")
{
    ScoreContext context;
    context.facility = "Trade";
    context.product = "Money";
    context.level = 3;
    context.slots = 1;
    context.dormitory_capacity = 20;
    context.selected_operator_ids = { "char_473_mberry", "char_2024_chyue" };
    const std::vector<ScoreOper> opers = {
        { { "bskill_tra_bd_n2" }, "char_455_nothin", "mr-nothing", 1.0 },
        { { "bskill_tra_spd3" }, "", "ordinary", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1 });

    context.use_worldly_plight = true;
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0 });
}

TEST_CASE("infrast abyssal hunter control operator is opt in", "[infrast][score][options]")
{
    ScoreContext context;
    context.facility = "Control";
    context.slots = 5;
    const std::vector<ScoreOper> opers = {
        { { "bskill_ctrl_aegir2", "bskill_ctrl_cost" }, "char_474_glady", "gladiia", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-a", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-b", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-c", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 1, 2, 3 });

    context.use_abyssal_hunter = true;
    const auto enabled = select_best_opers(opers, context);
    CHECK(std::ranges::find(enabled.indices, 0) != enabled.indices.end());
}

TEST_CASE("infrast manufacturing identifies pinus operators for control linkage", "[infrast][score][identity]")
{
    ScoreContext context;
    context.facility = "Mfg";
    context.product = "CombatRecord";
    context.level = 3;
    context.slots = 3;
    context.selected_operator_ids = { "char_4098_vvana", "char_420_flamtl" };
    const std::vector<ScoreOper> opers = {
        { { "bskill_man_spd2" }, "char_430_fartth", "fartooth", 1.0 },
        { { "bskill_man_spd2" }, "char_431_ashlok", "ashlock", 1.0 },
        { { "bskill_man_spd2" }, "char_496_wildmn", "wild-mane", 1.0 },
        { { "bskill_man_spd3" }, "", "generic-a", 1.0 },
        { { "bskill_man_spd3" }, "", "generic-b", 1.0 },
        { { "bskill_man_spd3" }, "", "generic-c", 1.0 },
    };

    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 3, 4, 5 });

    context.use_pinus_sylvestris = true;
    CHECK(select_best_opers(opers, context).indices == std::vector<size_t> { 0, 1, 2 });

    auto ambiguous_opers = opers;
    for (size_t index = 0; index < 3; ++index) {
        ambiguous_opers[index].operator_id.clear();
    }
    CHECK(select_best_opers(ambiguous_opers, context).indices == std::vector<size_t> { 3, 4, 5 });
}

TEST_CASE("infrast perception linkage propagates through every facility", "[infrast][score][identity]")
{
    ScoreContext context;
    context.facility = "Office";
    context.slots = 1;
    context.use_perception_information = true;
    const std::vector<ScoreOper> office_opers = {
        { { "bskill_hire_spd_bd_n1_n1", "bskill_hire_spd_memento" }, "char_436_whispr", "whisperain", 1.0 },
        { { "bskill_hire_skgoat2" }, "", "ordinary", 1.0 },
    };
    CHECK(select_best_opers(office_opers, context).indices == std::vector<size_t> { 0 });
    context.selected_operator_ids.emplace(office_opers[0].operator_id);

    context.facility = "Control";
    context.slots = 5;
    const std::vector<ScoreOper> control_opers = {
        { { "bskill_ctrl_cost_bd1", "bskill_ctrl_cost_bd2" }, "char_2015_dusk", "dusk", 1.0 },
        { { "bskill_ctrl_h_spd" }, "", "saileach", 1.0 },
        { { "bskill_ctrl_sp" }, "", "generic-a", 1.0 },
        { { "bskill_ctrl_cost" }, "", "generic-b", 1.0 },
        { { "bskill_ctrl_cost_expand" }, "", "generic-c", 1.0 },
    };
    const auto control_result = select_best_opers(control_opers, context);
    CHECK(std::ranges::find(control_result.indices, 0) != control_result.indices.end());
    context.selected_operator_ids.emplace(control_opers[0].operator_id);

    context.facility = "Mfg";
    context.product = "CombatRecord";
    context.level = 3;
    context.slots = 1;
    const std::vector<ScoreOper> mfg_opers = {
        { { "bskill_man_spd_bd2" }, "char_391_rosmon", "rosmontis", 1.0 },
        { { "bskill_man_spd3" }, "", "ordinary", 1.0 },
    };
    CHECK(select_best_opers(mfg_opers, context).indices == std::vector<size_t> { 0 });
    context.selected_operator_ids.emplace(mfg_opers[0].operator_id);

    context.facility = "Trade";
    context.product = "Money";
    context.dormitory_capacity = 20;
    const std::vector<ScoreOper> trade_opers = {
        { { "bskill_tra_spd_bd2" }, "char_4046_ebnhlz", "ebenholz", 1.0 },
        { { "bskill_tra_spd3" }, "", "ordinary", 1.0 },
    };
    CHECK(select_best_opers(trade_opers, context).indices == std::vector<size_t> { 0 });

    context.facility = "Dorm";
    context.slots = 3;
    const std::vector<ScoreOper> dorm_opers = {
        { { "bskill_dorm_bdnum" }, "char_245_cello", "virtuosa", 1.0 },
        { { "bskill_dorm_all&bd_n1", "bskill_dorm_all&bd_n1_n2" }, "char_338_iris", "iris", 1.0 },
        { { "bskill_dorm_all&bd_n1_2", "bskill_dorm_all&bd_n1_n3" }, "char_4047_pianst", "czerny", 1.0 },
        { { "bskill_dorm_all3" }, "", "ordinary", 1.0 },
    };
    CHECK(select_best_opers(dorm_opers, context).indices == std::vector<size_t> { 0, 1, 2 });
}
