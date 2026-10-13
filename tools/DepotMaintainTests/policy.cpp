#include "Common/DepotQuantity.h"
#include "Common/DepotTokenBoundary.h"
#include "Task/Fight/StageDropsGuard.h"
#include "Task/Miscellaneous/DepotMaintainExecution.h"
#include "Task/Miscellaneous/DepotMaintainPolicy.h"
#include "Task/Miscellaneous/DepotMaintainRecipeWire.h"
#include "Task/Miscellaneous/DepotMaintainRoutes.h"
#include "Task/Miscellaneous/DepotMaintainWire.h"
#include "Task/Miscellaneous/DepotScanGuard.h"
#include <cassert>
#include <iostream>
#include <random>

int main(int argc, char* argv[])
{
    using asst::depot_maintain::Inventory;
    using asst::depot_token::grid_geometry;
    assert(grid_geometry(1280, 720, 1072, 496, 146, 146)); // native failure receipt, bottom642
    assert(!grid_geometry(1280, 720, 1072, 496, 146, 147)); // no extra uncalibrated pixel
    assert(!grid_geometry(1280, 720, 1040, 646, 214, 51)); // storage button
    assert(!grid_geometry(2560, 1440, 1072, 496, 146, 146));
    assert(!grid_geometry(1280, 720, -1, 496, 146, 146));
    assert(!grid_geometry(1280, 720, 1072, 89, 146, 146));
    assert(!grid_geometry(1280, 720, 1072, 496, 0, 146));
    assert(!grid_geometry(1280, 720, 1072, 496, 146, -1));
    assert(!grid_geometry(1280, 720, 1072, 496, 2147483647, 146));
    assert(!grid_geometry(1280, 720, 2147483647, 496, 146, 146));
    using asst::depot_quantity::lmd_detail;
    assert(lmd_detail("龙门币", "库存", "589268") == 589'268);
    assert(lmd_detail("龙门币", "库存", "0") == 0);
    assert(!lmd_detail("合成玉", "库存", "589268"));
    assert(!lmd_detail("龙门币", "", "589268"));
    assert(!lmd_detail("龙门币", "库", "589268"));
    assert(!lmd_detail("龙门币", "库存", "58.9万"));
    assert(!lmd_detail("龙门币", "库存", "589,268"));
    assert(!lmd_detail("龙门币", "库存", "589268x"));
    assert(!lmd_detail("龙门币", "库存", "-1"));
    assert(!lmd_detail("龙门币", "库存", "2147483648"));
    assert(!lmd_detail("龙门币", "库存", ""));
    using asst::depot_token::detail;
    assert(detail("焰影苇草的信物", "用于提升焰影苇草的潜能。", "库存", "存入保险库"));
    assert(detail("星熊的信物", "用于提升星熊的潜能", "库存", "存入保险库"));
    assert(!detail("星熊的信物", "用于提升焰影苇草的潜能。", "库存", "存入保险库"));
    assert(!detail("星熊的信物", "用于提升星熊的潜能", "库存", "使用"));
    assert(!detail("星熊的信物", "用于提升星熊的潜能", "", "存入保险库"));
    assert(!detail("特种芯片", "用于提升星熊的潜能", "库存", "存入保险库"));
    assert(!detail("的信物", "用于提升的潜能", "库存", "存入保险库"));
    assert(!detail("", "", "库存", "存入保险库"));
    assert(detail("近卫皇家信物", "用于提升★6近卫干员的潜能", "库存", "采购中心"));
    assert(!detail("近卫皇家信物", "用于提升★6狙击干员的潜能", "库存", "采购中心"));
    assert(!detail("近卫皇家信物", "用于提升★5近卫干员的潜能", "库存", "采购中心"));
    assert(!detail("近卫皇家信物", "用于提升★6近卫干员的潜能", "库存", "购买"));
    assert(!detail("近卫芯片", "用于提升★6近卫干员的潜能", "库存", "采购中心"));
    using namespace asst::depot_maintain;
    using Scan = asst::DepotScanGuard;
    using ScanDecision = Scan::Decision;
    Scan good_scan;
    assert(good_scan.observe(true, 24) == ScanDecision::Continue);
    assert(good_scan.observe(true, 48) == ScanDecision::Continue);
    assert(good_scan.observe(true, 48, true) == ScanDecision::Complete);
    assert(good_scan.observe(true, 48) == ScanDecision::Reject);
    Scan changed_repeat;
    assert(changed_repeat.observe(true, 24, false) == ScanDecision::Continue);
    assert(changed_repeat.observe(true, 24, false) == ScanDecision::Reject);
    assert(changed_repeat.observe(true, 24, true) == ScanDecision::Reject);
    Scan matching_end;
    assert(matching_end.observe(true, 24, false) == ScanDecision::Continue);
    assert(matching_end.observe(true, 48, false) == ScanDecision::Continue);
    assert(matching_end.observe(true, 48, true) == ScanDecision::Complete);
    Scan unproved_repeat;
    assert(unproved_repeat.observe(true, 24) == ScanDecision::Continue);
    assert(unproved_repeat.observe(true, 24) == ScanDecision::Reject);
    assert(asst::depot_quantity::agrees(37, true, 37, true));
    assert(!asst::depot_quantity::agrees(3, true, 37, true));
    assert(!asst::depot_quantity::agrees(37, false, 37, true));
    assert(!asst::depot_quantity::agrees(37, true, 37, false));
    Scan interrupted_scan;
    assert(interrupted_scan.observe(true, 24) == ScanDecision::Continue);
    assert(interrupted_scan.observe(false, 24) == ScanDecision::Reject);
    assert(interrupted_scan.observe(true, 24) == ScanDecision::Reject);
    Scan empty_scan;
    assert(empty_scan.observe(false, 0) == ScanDecision::Reject);
    Scan npos_scan;
    assert(npos_scan.observe(true, std::numeric_limits<size_t>::max()) == ScanDecision::Reject);
    Scan backwards_scan;
    assert(backwards_scan.observe(true, 24) == ScanDecision::Continue);
    assert(backwards_scan.observe(true, 48) == ScanDecision::Continue);
    assert(backwards_scan.observe(true, 24) == ScanDecision::Reject);
    Scan bounded_scan;
    for (size_t page = 1; page <= Scan::MaxPages; ++page) {
        assert(bounded_scan.observe(true, page) == ScanDecision::Continue);
    }
    assert(bounded_scan.observe(true, Scan::MaxPages + 1) == ScanDecision::Reject);
    std::cout
        << "PASS strict depot scan: valid end, partial failure, empty/NPos, backwards, terminal and page budget\n";
    asst::StageDropsGuard drop_guard;
    assert(drop_guard.allows_next_battle(true));
    assert(drop_guard.allows_next_battle(false)); // Ordinary Fight unchanged.
    int validated = 0;
    bool ledger_valid = true;
    drop_guard.set([&]() {
        ++validated;
        return ledger_valid;
    });
    assert(!drop_guard.allows_next_battle(false));
    assert(validated == 0); // Never validate an unrecognized result.
    assert(drop_guard.allows_next_battle(true));
    assert(validated == 1);
    ledger_valid = false;
    assert(!drop_guard.allows_next_battle(true));
    assert(validated == 2);
    assert(!drop_guard.allows_next_battle(false));
    assert(validated == 2);
    drop_guard.set({});
    assert(drop_guard.allows_next_battle(false));
    assert(route_evidence("1-7", "main_01-07", 6, { "30012", "30011" }, "30012") == RouteEvidence::ListedDrop);
    assert(route_evidence("1-7", "main_01-07", 6, { "30012" }, "30135") == RouteEvidence::ItemNotListed);
    assert(!route_supported(route_evidence("1-7", "main_01-07", 6, { "30012" }, "30135")));
    assert(route_evidence("CE-6", "wk_melee_6", 36, {}, "4001") == RouteEvidence::FixedResourceReward);
    assert(route_evidence("AP-5", "wk_toxic_5", 30, {}, "4006") == RouteEvidence::FixedResourceReward);
    assert(route_evidence("CE-6", "wk_toxic_5", 30, {}, "4001") == RouteEvidence::ItemNotListed);
    assert(route_evidence("AP-5", "wk_toxic_5", 30, {}, "32001") == RouteEvidence::ItemNotListed);
    assert(route_evidence("AP-50", "wk_toxic_50", 30, {}, "4006") == RouteEvidence::ItemNotListed);
    assert(route_evidence("CE-6suffix", "wk_melee_6", 36, {}, "4001") == RouteEvidence::ItemNotListed);
    assert(route_evidence("CE-6", "wk_melee_6", 0, { "4001" }, "4001") == RouteEvidence::StageUnknown);
    assert(route_evidence("1-7", "", 6, { "30012" }, "30012") == RouteEvidence::StageUnknown);
    assert(!known_natural_stage("main_01-07", -1));
    assert(!route_supported(RouteEvidence::StageUnknown));
    for (int level = 1; level <= 6; ++level) {
        assert(route_supported(
            route_evidence("CE-" + std::to_string(level), "wk_melee_" + std::to_string(level), 10, {}, "4001")));
    }
    for (int level = 1; level <= 5; ++level) {
        assert(route_supported(
            route_evidence("AP-" + std::to_string(level), "wk_toxic_" + std::to_string(level), 10, {}, "4006")));
    }
    Inventory route_inventory;
    assert(route_inventory.replace_fresh({ { "bad_route_goal", 1 }, { "other", 20 } }, true, true));
    std::vector<Goal> route_goals { { "bad_route_goal", 20, true, false }, { "other", 20, true, true } };
    assert(completion(route_inventory, route_goals, true) == Completion::Unresolved);
    assert(route_inventory.add_actual_drop("bad_route_goal", 19));
    assert(completion(route_inventory, route_goals, true) == Completion::AllMet);
    Inventory inv;
    assert(!inv.count("a"));
    assert(!inv.replace_fresh({ { "a", 999 } }, false, true));
    assert(!inv.count("a"));
    assert(!inv.replace_fresh({ { "a", 999 } }, true, false));
    assert(inv.replace_fresh({ { "a", 5 }, { "b", 20 }, { "zero", 0 } }, true, true));
    assert(inv.shortage("a", 20) == 15);
    assert(inv.shortage("b", 20) == 0);
    assert(inv.shortage("zero", 20) == 20);
    assert(!inv.shortage("unseen", 20));
    assert(!inv.shortage("a", 0));
    assert(!inv.shortage("a", -1));
    assert(inv.add_actual_drop("a", 12));
    assert(inv.shortage("a", 20) == 3);
    assert(inv.add_actual_drop("a", 4));
    assert(inv.shortage("a", 20) == 0);
    assert(inv.add_actual_drop("unseen", 1));
    assert(!inv.count("unseen"));
    assert(!inv.add_actual_drop("a", -1));
    assert(!inv.add_actual_drop("", 1));
    assert(inv.replace_fresh({ { "a", 1 } }, true, true));
    assert(inv.shortage("a", 20) == 19);
    assert(!inv.count("b"));
    assert(!inv.replace_fresh({ { "a", -1 } }, true, true));
    assert(!inv.count("a"));
    assert(inv.replace_fresh({ { "a", INT64_MAX } }, true, true));
    assert(!inv.add_actual_drop("a", 1));
    assert(!inv.count("a"));
    using asst::depot_maintain::depot_counts;
    assert(depot_counts("{\"a\":20}")->at("a") == 20);
    assert(depot_counts("{\"a\":0}")->at("a") == 0);
    assert(!depot_counts("{\"a\":1.5}"));
    assert(!depot_counts("{\"a\":\"20\"}"));
    assert(!depot_counts("{\"a\":true}"));
    assert(!depot_counts("{\"a\":-1}"));
    assert(!depot_counts("{\"a\":9223372036854775808}"));
    assert(!depot_counts("[20]"));
    assert(!depot_counts("not json"));
    assert(depot_counts("{}")->empty());
    // Exercise the same full-snapshot consumer invoked by the nested callback.
    const auto depot_event = [](const std::string& payload, bool done) {
        return json::object { { "details", json::object { { "verified_data", payload }, { "done", done } } } };
    };
    Inventory::Counts received;
    bool received_done = false;
    assert(consume_depot_snapshot(received, received_done, depot_event("{\"30012\":20,\"2001\":293}", false)));
    assert(received.at("30012") == 20 && !received_done);
    // Overlap agrees: retain stock without double counting.
    assert(consume_depot_snapshot(received, received_done, depot_event("{\"30012\":20,\"2001\":293}", false)));
    assert(received.at("30012") == 20 && received.size() == 2);
    // Conflicting or unverified overlap withdraws its exact quantity.
    assert(consume_depot_snapshot(received, received_done, depot_event("{\"2001\":293}", false)));
    assert(!received.contains("30012") && received.at("2001") == 293);
    // New IDs and final exact LMD remain present in the cumulative snapshot.
    assert(consume_depot_snapshot(received, received_done, depot_event("{\"2001\":293,\"3303\":10}", false)));
    assert(received.at("3303") == 10 && !received.contains("30012"));
    assert(consume_depot_snapshot(
        received,
        received_done,
        depot_event("{\"2001\":293,\"3303\":10,\"4001\":589268}", true)));
    assert(received_done && received.at("4001") == 589'268 && !received.contains("30012"));
    Inventory final_snapshot;
    assert(final_snapshot.replace_fresh(received, true, received_done));
    assert(!final_snapshot.shortage("30012", 20));
    assert(completion(final_snapshot, { { "30012", 20, true, true } }, false) == Completion::Unresolved);
    assert(final_snapshot.add_actual_drop("30012", 1));
    assert(!final_snapshot.count("30012"));
    // Invalid payloads cannot retain a previous exact observation or done flag.
    assert(!consume_depot_snapshot(received, received_done, depot_event("{\"2001\":\"293\"}", true)));
    assert(received.empty() && !received_done);
    assert(consume_depot_snapshot(received, received_done, depot_event("{\"2001\":293}", true)));
    assert(!consume_depot_snapshot(
        received,
        received_done,
        json::object { { "details", json::object { { "verified_data", "{}" }, { "done", "true" } } } }));
    assert(received.empty() && !received_done);
    assert(consume_depot_snapshot(received, received_done, depot_event("{}", true)));
    assert(received.empty() && received_done);
    // GUI stock receipts come from the exact Core ledger, never legacy stats.
    Inventory receipt_stock;
    assert(receipt_stock.exact_counts().empty());
    assert(receipt_stock.replace_fresh({ { "30012", 20 }, { "2001", 293 } }, true, true));
    bool receipt_invalid = false;
    const auto receipt_event = *json::parse(
        "{\"details\":{\"stage\":{\"stageCode\":\"1-7\",\"stageId\":\"main_01-07\"},"
        "\"drops\":[{\"itemId\":\"30012\",\"quantity\":1},{\"itemId\":\"unseen\",\"quantity\":1}],"
        "\"stats\":[{\"itemId\":\"30012\",\"quantity\":999}]}}");
    const auto receipt_event_before = receipt_event.to_string();
    auto inventory_receipt = accept_stage_inventory(receipt_stock, receipt_event, "1-7", "main_01-07", receipt_invalid);
    assert(inventory_receipt && !receipt_invalid);
    assert(inventory_receipt->at("30012").as_integer() == 21 && inventory_receipt->at("2001").as_integer() == 293);
    assert(!inventory_receipt->contains("unseen"));
    assert(receipt_event.to_string() == receipt_event_before);
    assert(!receipt_event.get("details", json::object {}).contains("verified_inventory"));
    assert(!accept_stage_inventory(receipt_stock, receipt_event, "1-8", "main_01-08", receipt_invalid));
    assert(receipt_invalid && receipt_stock.count("30012") == 21);
    assert(!accept_stage_inventory(receipt_stock, receipt_event, "1-7", "main_01-07", receipt_invalid));
    assert(receipt_invalid);
    Inventory invalid_receipt_stock;
    assert(invalid_receipt_stock.replace_fresh({ { "30012", INT64_MAX } }, true, true));
    bool invalid_receipt = false;
    assert(!accept_stage_inventory(invalid_receipt_stock, receipt_event, "1-7", "main_01-07", invalid_receipt));
    assert(invalid_receipt);
    assert(!invalid_receipt_stock.add_actual_drop("30012", 1));
    assert(invalid_receipt_stock.exact_counts().empty());
    // One verified shortage stops the task; a cheaper next route cannot
    // convert this stop decision into permission to continue or run fallback.
    assert(shortage_fight_decision(1, true) == ShortageFightDecision::NaturalBudgetStop);
    assert(shortage_fight_decision(100, true) == ShortageFightDecision::NaturalBudgetStop);
    assert(shortage_fight_decision(1, false) == ShortageFightDecision::UnverifiedStop);
    assert(shortage_fight_decision(0, false) == ShortageFightDecision::TargetMet);
    assert(shortage_fight_decision(0, true) == ShortageFightDecision::TargetMet);
    assert(shortage_fight_decision(-1, true) == ShortageFightDecision::UnverifiedStop);
    std::cout << "PASS full DepotInfo snapshots: withdrawal, unknown fallback block, new item, final LMD and "
                 "natural-budget stop\n";
    auto details = *json::parse("{\"details\":{\"drops\":[{\"itemId\":\"a\",\"quantity\":2}]}}");
    auto drops = details.get("details", json::object {}).find<json::array>("drops");
    assert(drops && drops->size() == 1);
    assert(asst::depot_maintain::recognized_count((*drops)[0].get("quantity", json::value())) == 2);
    using asst::depot_maintain::natural_budget_blocked;
    assert(natural_budget_blocked(
        *json::parse("{\"what\":\"ExceededLimit\",\"details\":{\"task\":\"Fight@MedicineConfirm\",\"max_times\":0}}")));
    assert(natural_budget_blocked(
        *json::parse("{\"what\":\"ExceededLimit\",\"details\":{\"task\":\"StoneConfirm\",\"max_times\":0}}")));
    assert(!natural_budget_blocked(
        *json::parse("{\"what\":\"ExceededLimit\",\"details\":{\"task\":\"StartButton1\",\"max_times\":0}}")));
    assert(!natural_budget_blocked(
        *json::parse("{\"what\":\"OtherEvent\",\"details\":{\"task\":\"MedicineConfirm\",\"max_times\":0}}")));
    assert(!natural_budget_blocked(
        *json::parse("{\"what\":\"ExceededLimit\",\"details\":{\"task\":\"MedicineConfirm\",\"max_times\":1}}")));
    using asst::depot_maintain::valid_params;
    assert(valid_params(*json::parse("{\"plans\":[]}")));
    assert(valid_params(*json::parse("{\"plans\":[{\"enabled\":false,\"medicine\":2,\"stone\":1,\"medicine_expire_days\":2}]}")));
    for (const auto* key : { "medicine", "stone", "medicine_expire_days" }) {
        for (const auto* invalid : { "-1", "true", "1.5", "\"2\"", "10000" }) {
            const auto payload = std::string("{\"plans\":[{\"enabled\":false,\"") + key + "\":" + invalid + "}]}";
            assert(!valid_params(*json::parse(payload)));
        }
    }
    assert(!valid_params(*json::parse("{\"plans\":[{\"enabled\":false,\"medicine_expire_days\":3}]}")));
    const auto limited = *json::parse("{\"what\":\"ExceededLimit\",\"details\":{\"task\":\"Fight@StoneConfirm\",\"max_times\":2}}");
    assert(recovery_budget_blocked(limited, 0, 2, 0));
    assert(!recovery_budget_blocked(limited, 0, 1, 0));
    assert(!recovery_budget_blocked(*json::parse("{\"what\":\"StageDrops\"}"), 0, 0, 0));
    RecoveryStopEvidence recovery_stop;
    auto terminal = *json::parse("{\"subtask\":\"ProcessTask\",\"details\":{\"task\":\"Fight@CloseStonePageExceeded\"}}");
    recovery_stop.observe(limited, false, true);
    assert(!recovery_stop.verified_stop());
    recovery_stop.observe(terminal, false, false);
    assert(!recovery_stop.verified_stop());
    recovery_stop.observe(terminal, true, false);
    assert(recovery_stop.verified_stop());
    recovery_stop.observe(*json::parse("{\"subtask\":\"ProcessTask\",\"what\":\"ExceededLimit\",\"details\":{\"task\":\"Fight@StoneConfirmWait\"}}"), false, true);
    assert(!recovery_stop.verified_stop());
    recovery_stop.observe(terminal, true, false);
    assert(!recovery_stop.verified_stop());
    recovery_stop = {};
    assert(!recovery_stop.verified_stop());
    terminal["subtask"] = "OtherTask";
    recovery_stop.observe(terminal, true, false);
    assert(!recovery_stop.verified_stop());
    assert(valid_params(*json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":20,\"stage\":\"1-7\"}]}")));
    assert(valid_params(*json::parse("{\"plans\":[{\"enabled\":false}]}")));
    assert(!valid_params(*json::parse("{\"plans\":[{\"item_id\":20,\"target\":20,\"stage\":\"1-7\"}]}")));
    assert(!valid_params(*json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":20,\"stage\":true}]}")));
    assert(!valid_params(*json::parse("{\"plans\":[{\"enabled\":\"false\"}]}")));
    assert(!valid_params(
        *json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":20,\"stage\":\"1-7\",\"stage_open\":1}]}")));
    assert(!valid_params(*json::parse("{\"plans\":[],\"fallback_stage\":1}")));
    assert(!valid_params(*json::parse("{\"plans\":[],\"fallback_open\":\"true\"}")));
    assert(!valid_params(*json::parse("{\"plans\":[],\"only_first\":1}")));
    assert(!valid_params(*json::parse("{\"plans\":[],\"fight_options\":[]}")));
    assert(!valid_params(*json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":1.5,\"stage\":\"1-7\"}]}")));
    assert(!valid_params(*json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":2147483648,\"stage\":\"1-7\"}]}")));
    assert(!valid_params(*json::parse("[]")));
    using asst::depot_quantity::exact_ascii;
    assert(exact_ascii("0", 1) == 0);
    assert(exact_ascii("269", 1) == 269);
    assert(!exact_ascii("", 1));
    assert(!exact_ascii("O", 1));
    assert(!exact_ascii("80.5", 10'000));
    assert(!exact_ascii("20", 10'000));
    assert(!exact_ascii("20.0", 1));
    assert(!exact_ascii("-1", 1));
    assert(!exact_ascii("2147483648", 1));
    assert(!exact_ascii("1/20", 1));
    using asst::depot_maintain::completion;
    using asst::depot_maintain::Completion;
    using asst::depot_maintain::Goal;
    std::vector<Goal> goals { { "a", 20, true, false }, { "b", 20, true, true } };
    assert(inv.replace_fresh({ { "a", 19 }, { "b", 19 } }, true, true));
    assert(completion(inv, goals, false) == Completion::Unresolved);
    assert(completion(inv, goals, true) == Completion::Unresolved);
    assert(inv.add_actual_drop("a", 1)); // Later stage completes earlier closed-stage goal.
    assert(completion(inv, goals, false) == Completion::UnverifiedStop);
    assert(completion(inv, goals, true) == Completion::NaturalBudgetIncomplete);
    assert(inv.add_actual_drop("b", 1));
    assert(completion(inv, goals, true) == Completion::AllMet);
    assert(completion(inv, goals, false) == Completion::AllMet);
    goals.push_back({ "unseen", 20, true, true });
    assert(completion(inv, goals, true) == Completion::Unresolved);
    goals.back().enabled = false;
    assert(completion(inv, goals, true) == Completion::AllMet);
    assert(!inv.replace_fresh({ { "a", 99 }, { "b", 99 } }, false, true));
    assert(completion(inv, goals, true) == Completion::Unresolved);
    Inventory co_drop;
    std::vector<Goal> co_goals { { "30012", 1760, true, true }, { "2001", 292, true, false } };
    assert(co_drop.replace_fresh({ { "30012", 1759 }, { "2001", 291 } }, true, true));
    assert(co_drop.shortage("30012", 1760) == 1);
    assert(co_drop.shortage("2001", 292) == 1);
    assert(completion(co_drop, co_goals, false) == Completion::Unresolved);
    // Actual StageDrops updates every recognized item, including a later
    // priority whose own stage is closed. No estimated drop/run count.
    assert(co_drop.add_actual_drop("30012", 1));
    assert(co_drop.shortage("30012", 1760) == 0);
    assert(co_drop.shortage("2001", 292) == 1);
    assert(completion(co_drop, co_goals, false) == Completion::Unresolved);
    assert(co_drop.add_actual_drop("2001", 1));
    assert(co_drop.shortage("2001", 292) == 0);
    assert(completion(co_drop, co_goals, false) == Completion::AllMet);
    assert(completion(co_drop, co_goals, true) == Completion::AllMet);
    assert(co_drop.count("2001") == 292);
    // A fresh run replaces prior drops; yesterday's co-drop is not inventory.
    assert(co_drop.replace_fresh({ { "30012", 1760 }, { "2001", 291 } }, true, true));
    assert(co_drop.shortage("2001", 292) == 1);
    assert(completion(co_drop, co_goals, true) == Completion::Unresolved);
    using asst::depot_maintain::apply_actual_drops;
    using asst::depot_maintain::apply_actual_stage_drops;
    Inventory staged;
    assert(staged.replace_fresh({ { "a", 5 }, { "b", 10 } }, true, true));
    const auto matching_result = *json::parse(
        "{\"details\":{\"stage\":{\"stageCode\":\"1-7\",\"stageId\":\"main_01-07\"},"
        "\"drops\":[{\"itemId\":\"a\",\"quantity\":2},{\"itemId\":\"b\",\"quantity\":3}]}}");
    assert(apply_actual_stage_drops(staged, matching_result, "1-7", "main_01-07"));
    assert(staged.count("a") == 7 && staged.count("b") == 13);
    for (const auto& [code, id] : std::vector<std::pair<std::string, std::string>> { { "", "main_01-07" },
                                                                                     { "1-7", "" },
                                                                                     { "LS-6", "main_01-07" },
                                                                                     { "1-7", "wrong_stage" },
                                                                                     { "1-7", "main_01-07#tough" } }) {
        assert(!apply_actual_stage_drops(staged, matching_result, code, id));
        assert(staged.count("a") == 7 && staged.count("b") == 13);
    }
    for (const auto& bad_stage : { "null",
                                   "[]",
                                   "{}",
                                   "true",
                                   "1",
                                   "\"1-7\"",
                                   "{\"stageCode\":1,\"stageId\":\"main_01-07\"}",
                                   "{\"stageCode\":\"1-7\",\"stageId\":true}",
                                   "{\"stageCode\":\"1-7\"}" }) {
        const auto result = *json::parse(
            std::string("{\"details\":{\"stage\":") + bad_stage + ",\"drops\":[{\"itemId\":\"a\",\"quantity\":2}]}}");
        assert(!apply_actual_stage_drops(staged, result, "1-7", "main_01-07"));
        assert(staged.count("a") == 7 && staged.count("b") == 13);
    }
    assert(!apply_actual_stage_drops(staged, *json::parse("{}"), "1-7", "main_01-07"));
    assert(!apply_actual_stage_drops(staged, *json::parse("{\"details\":{\"drops\":[]}}"), "1-7", "main_01-07"));
    auto malformed_drop = matching_result;
    malformed_drop["details"]["drops"] =
        *json::parse("[{\"itemId\":\"a\",\"quantity\":2},{\"itemId\":\"b\",\"quantity\":-1}]");
    assert(!apply_actual_stage_drops(staged, malformed_drop, "1-7", "main_01-07"));
    assert(staged.count("a") == 7 && staged.count("b") == 13);
    Inventory transaction;
    assert(transaction.replace_fresh({ { "a", 5 }, { "b", 10 } }, true, true));
    assert(apply_actual_drops(
        transaction,
        *json::parse(
            "{\"details\":{\"drops\":[{\"itemId\":\"a\",\"quantity\":2},{\"itemId\":\"b\",\"quantity\":3}]}}")));
    assert(transaction.count("a") == 7);
    assert(transaction.count("b") == 13);
    for (const auto& bad : { "null",
                             "[]",
                             "{}",
                             "{\"itemId\":1,\"quantity\":2}",
                             "{\"itemId\":\"b\",\"quantity\":-1}",
                             "{\"itemId\":\"b\",\"quantity\":1.5}",
                             "{\"itemId\":\"b\",\"quantity\":\"2\"}",
                             "{\"itemId\":\"\",\"quantity\":2}" }) {
        const auto result =
            *json::parse(std::string("{\"details\":{\"drops\":[{\"itemId\":\"a\",\"quantity\":2},") + bad + "]}}");
        assert(!apply_actual_drops(transaction, result));
        assert(transaction.count("a") == 7);
        assert(transaction.count("b") == 13);
    }
    assert(!apply_actual_drops(transaction, *json::parse("{\"details\":{\"drops\":{}}}")));
    assert(!apply_actual_drops(transaction, *json::parse("{}")));
    assert(apply_actual_drops(transaction, *json::parse("{\"details\":{\"drops\":[]}}")));
    assert(apply_actual_drops(
        transaction,
        *json::parse("{\"details\":{\"drops\":[{\"itemId\":\"unseen\",\"quantity\":2}]}}")));
    assert(!transaction.count("unseen"));
    assert(transaction.replace_fresh({ { "a", 5 }, { "b", INT64_MAX } }, true, true));
    assert(!apply_actual_drops(
        transaction,
        *json::parse(
            "{\"details\":{\"drops\":[{\"itemId\":\"a\",\"quantity\":2},{\"itemId\":\"b\",\"quantity\":1}]}}")));
    assert(transaction.count("a") == 5);
    assert(transaction.count("b") == INT64_MAX);
    using asst::depot_maintain::is_drop_recognition_task;
    assert(is_drop_recognition_task(*json::parse("{\"subtask\":\"StageDropsTask\"}")));
    assert(!is_drop_recognition_task(*json::parse("{\"subtask\":\"ReportDataTask\"}")));
    assert(!is_drop_recognition_task(*json::parse("{}")));
    using asst::depot_maintain::fallback_budget_verified;
    assert(fallback_budget_verified(true, true));
    assert(!fallback_budget_verified(true, false));
    assert(!fallback_budget_verified(false, true));
    assert(!fallback_budget_verified(false, false));
    using asst::depot_maintain::preview_dependencies;
    using asst::depot_maintain::recipe_catalog;
    using asst::depot_maintain::Recipes;
    const Recipes recipes { { "a", { { "raw", 2 } } },
                            { "b", { { "raw", 2 } } },
                            { "top", { { "a", 3 }, { "b", 1 } } } };
    Inventory stock;
    assert(stock.replace_fresh({ { "a", 0 }, { "b", 0 }, { "raw", 10 }, { "top", 0 } }, true, true));
    std::vector<Goal> reserve_goals { { "a", 2, true, false }, { "b", 3, true, false } };
    auto preview = preview_dependencies(stock, recipes, reserve_goals, {});
    assert(preview.verified());
    assert(preview.crafts.at("a") == 2 && preview.crafts.at("b") == 3);
    assert(preview.allocated_stock.at("raw") == 10);
    assert(preview.farm.empty());
    assert(stock.count("raw") == 10); // A preview cannot spend actual inventory.
    reserve_goals.push_back({ "raw", 4, true, false });
    preview = preview_dependencies(stock, recipes, reserve_goals, {});
    assert(preview.verified() && preview.protected_stock.at("raw") == 4);
    assert(preview.allocated_stock.at("raw") == 6 && preview.farm.at("raw") == 4);
    reserve_goals.push_back({ "raw", 3, true, false }); // Duplicate reserve uses MAX, not SUM.
    assert(preview_dependencies(stock, recipes, reserve_goals, {}).farm.at("raw") == 4);
    reserve_goals.push_back({ "unseen", 100, false, false });
    assert(preview_dependencies(stock, recipes, reserve_goals, {}).verified());
    reserve_goals.back().enabled = true;
    preview = preview_dependencies(stock, recipes, reserve_goals, {});
    assert(!preview.verified() && preview.error == "unknown_inventory");
    assert(preview.unknown.contains("unseen") && preview.farm.empty() && preview.crafts.empty());
    assert(stock.replace_fresh({ { "a", 1 }, { "b", 5 }, { "raw", 0 }, { "top", 0 } }, true, true));
    preview = preview_dependencies(stock, recipes, { { "top", 2, true, false } }, {});
    assert(preview.verified() && preview.crafts.at("top") == 2 && preview.crafts.at("a") == 5);
    assert(!preview.crafts.contains("b") && preview.allocated_stock.at("b") == 2);
    assert(preview.allocated_stock.at("a") == 1 && preview.farm.at("raw") == 10);
    // Keep three A in addition to consuming six A to make two TOP.
    preview = preview_dependencies(stock, recipes, { { "top", 2, true, false }, { "a", 3, true, false } }, {});
    assert(preview.verified() && preview.crafts.at("a") == 8 && preview.farm.at("raw") == 16);
    assert(preview.protected_stock.at("a") == 1 && !preview.allocated_stock.contains("a"));
    preview = preview_dependencies(stock, recipes, { { "top", 2, true, false } }, { "a" });
    assert(preview.verified() && preview.farm.at("a") == 5 && !preview.crafts.contains("a"));
    assert(!preview.farm.contains("raw")); // Explicit direct acquisition cuts the recipe graph.
    assert(stock.replace_fresh({ { "top", 2 } }, true, true));
    assert(preview_dependencies(stock, recipes, { { "top", 2, true, false } }, {}).verified());
    assert(preview_dependencies(stock, recipes, { { "top", 2, true, false } }, {}).farm.empty());
    assert(stock.replace_fresh({ { "a", 0 }, { "b", 0 }, { "raw", 0 } }, true, true));
    const Recipes cycle { { "a", { { "b", 1 } } }, { "b", { { "a", 1 } } } };
    preview = preview_dependencies(stock, cycle, { { "a", 1, true, false } }, {});
    assert(!preview.verified() && preview.error == "recipe_cycle" && preview.crafts.empty());
    assert(preview_dependencies(stock, cycle, { { "a", 1, true, false } }, { "a" }).farm.at("a") == 1);
    for (const auto count : { int64_t(0), int64_t(-1) }) {
        preview = preview_dependencies(stock, { { "a", { { "raw", count } } } }, { { "a", 1, true, false } }, {});
        assert(preview.error == "invalid_recipe");
    }
    assert(preview_dependencies(stock, { { "a", {} } }, { { "a", 1, true, false } }, {}).error == "invalid_recipe");
    preview = preview_dependencies(stock, { { "a", { { "raw", INT64_MAX } } } }, { { "a", 2, true, false } }, {});
    assert(!preview.verified() && preview.error == "requirement_overflow");
    assert(preview.farm.empty() && preview.crafts.empty());
    preview = preview_dependencies(
        stock,
        { { "a", { { "raw", INT64_MAX } } }, { "b", { { "raw", 1 } } } },
        { { "a", 1, true, false }, { "b", 1, true, false } },
        {});
    assert(preview.error == "requirement_overflow");
    assert(stock.replace_fresh({ { "a", 0 } }, true, true));
    preview = preview_dependencies(stock, recipes, { { "a", 1, true, false } }, {});
    assert(preview.error == "unknown_inventory" && preview.unknown.contains("raw"));
    assert(preview.crafts.empty() && preview.farm.empty() && preview.allocated_stock.empty());
    assert(preview_dependencies(stock, recipes, { { "a", 0, true, false } }, {}).error == "invalid_goal");
    assert(preview_dependencies(stock, recipes, { { "", 1, true, false } }, {}).error == "invalid_goal");
    assert(
        preview_dependencies(stock, recipes, std::vector<Goal>(257, { "a", 1, true, false }), {}).error ==
        "goal_limit");
    Recipes deep;
    for (int n = 0; n < 18; ++n) {
        deep["chain" + std::to_string(n)] = { { "chain" + std::to_string(n + 1), 1 } };
    }
    assert(stock.replace_fresh({ { "chain0", 0 } }, true, true));
    assert(preview_dependencies(stock, deep, { { "chain0", 1, true, false } }, {}).error == "recipe_depth_limit");
    Recipes wide;
    std::vector<Goal> many;
    Inventory::Counts wide_stock;
    for (int n = 0; n < 128; ++n) {
        const auto id = "parent" + std::to_string(n);
        many.push_back({ id, 1, true, false });
        wide_stock[id] = 0;
        for (int k = 0; k < 8; ++k) {
            wide[id][id + "child" + std::to_string(k)] = 1;
        }
    }
    assert(stock.replace_fresh(wide_stock, true, true));
    assert(preview_dependencies(stock, wide, many, {}).error == "recipe_node_limit");
    const std::unordered_set<std::string> known { "a", "raw" };
    assert(recipe_catalog(*json::parse("{\"a\":{\"raw\":3}}"), known)->at("a").at("raw") == 3);
    for (const auto& invalid : { "[]",
                                 "{\"a\":null}",
                                 "{\"a\":{}}",
                                 "{\"unknown\":{\"raw\":1}}",
                                 "{\"a\":{\"unknown\":1}}",
                                 "{\"a\":{\"raw\":0}}",
                                 "{\"a\":{\"raw\":-1}}",
                                 "{\"a\":{\"raw\":true}}",
                                 "{\"a\":{\"raw\":\"3\"}}",
                                 "{\"a\":{\"raw\":3.5}}",
                                 "{\"a\":{\"raw\":2147483648}}" }) {
        assert(!recipe_catalog(*json::parse(invalid), known));
    }
    assert(valid_params(*json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":2}],\"dependency_preview\":true}")));
    assert(!valid_params(*json::parse("{\"plans\":[{\"item_id\":\"a\",\"target\":2}]}")));
    assert(!valid_params(*json::parse("{\"plans\":[],\"dependency_preview\":\"true\"}")));
    auto receipt = asst::depot_maintain::dependency_receipt(preview);
    assert(!receipt.get("targets_met", true) && !receipt.get("craft_executed", true));
    assert(!receipt.get("verified", true));
    // Conservation check over varied DAGs/stocks: hypothetical crafting must
    // never spend protected reserves or allocate the same shared stock twice.
    std::mt19937 random(20'261'010);
    for (int sample = 0; sample < 1000; ++sample) {
        const Recipes graph { { "p", { { "m", 2 }, { "raw", 1 } } },
                              { "q", { { "m", 3 } } },
                              { "m", { { "raw", 2 } } } };
        Inventory::Counts initial;
        for (const auto& id : { "p", "q", "m", "raw" }) {
            initial[id] = static_cast<int64_t>(random() % 31);
        }
        Inventory observed;
        assert(observed.replace_fresh(initial, true, true));
        std::vector<Goal> requested;
        for (const auto& id : { "p", "q", "m", "raw" }) {
            requested.push_back({ id, static_cast<int64_t>(random() % 30) + 1, (random() % 2) == 0, false });
        }
        const std::set<std::string> direct = sample % 2 == 0 ? std::set<std::string> { "m" } : std::set<std::string> {};
        auto computed = preview_dependencies(observed, graph, requested, direct);
        assert(computed.verified());
        auto balance = initial;
        for (const auto& [id, quantity] : computed.farm) {
            balance[id] += quantity;
        }
        for (const auto& [id, quantity] : computed.crafts) {
            balance[id] += quantity;
            for (const auto& [ingredient, count] : graph.at(id)) {
                balance[ingredient] -= quantity * count;
            }
        }
        for (const auto& [id, quantity] : balance) {
            assert(quantity >= 0);
            assert(observed.count(id) == initial.at(id));
        }
        for (const auto& goal : requested) {
            assert(!goal.enabled || balance.at(goal.item) >= goal.target);
        }
    }
    if (argc >= 2) {
        // Exercise current native resources, not only a synthetic recipe graph.
        auto index = json::open(argv[1]);
        assert(index && index->is_object());
        json::value formulas = json::object {};
        std::unordered_set<std::string> ids;
        for (const auto& [id, item] : index->as_object()) {
            ids.emplace(id);
            if (const auto formula = item.find("formula")) {
                formulas[id] = *formula;
            }
        }
        const auto native = recipe_catalog(formulas, ids);
        assert(native && native->size() > 0);
        assert(native->at("30012").at("30011") == 3);
        assert(native->at("30013").at("30012") == 5);
        assert(native->at("30135").at("30104") == 1);
        assert(native->at("3213").at("32001") == 1);
        assert(native->at("3213").at("3212") == 2);
        assert(stock.replace_fresh({ { "30013", 0 }, { "30012", 2 }, { "30011", 1 } }, true, true));
        preview = preview_dependencies(stock, *native, { { "30013", 1, true, false } }, {});
        assert(preview.verified() && preview.crafts.at("30013") == 1 && preview.crafts.at("30012") == 3);
        assert(preview.farm.at("30011") == 8);
        Inventory::Counts empty_native;
        for (const auto& id : ids) {
            empty_native[id] = 0;
        }
        std::vector<Goal> every_recipe;
        for (const auto& [id, ingredients] : *native) {
            (void)ingredients;
            every_recipe.push_back({ id, 1, true, false });
        }
        assert(stock.replace_fresh(empty_native, true, true));
        preview = preview_dependencies(stock, *native, every_recipe, {});
        assert(preview.verified());
        auto balance = empty_native;
        for (const auto& [id, quantity] : preview.farm) {
            balance[id] += quantity;
        }
        for (const auto& [id, quantity] : preview.crafts) {
            balance[id] += quantity;
            for (const auto& [ingredient, count] : native->at(id)) {
                balance[ingredient] -= quantity * count;
            }
        }
        for (const auto& [id, quantity] : balance) {
            assert(quantity >= 0);
            assert(stock.count(id) == 0);
        }
        for (const auto& goal : every_recipe) {
            assert(balance.at(goal.item) >= goal.target);
        }
        assert(preview.crafts.at("3213") == 1 && preview.farm.at("32001") == 8);
        std::cout << "PASS current native recipe catalog: " << native->size() << " formulas\n";
    }
    if (argc >= 3) {
        auto stages = json::open(argv[2]);
        assert(stages && stages->is_array());
        int checked = 0;
        for (const auto& entry : stages->as_array()) {
            const auto code = entry.get("code", "");
            const auto id = entry.get("stageId", "");
            const auto cost = entry.get("apCost", 0);
            std::string target;
            if (code == "1-7") {
                target = "30012";
            }
            else if (code == "CE-6") {
                target = "4001";
            }
            else if (code == "LS-6") {
                target = "2004";
            }
            else if (code == "CA-5") {
                target = "3303";
            }
            else if (code == "AP-5") {
                target = "4006";
            }
            else if (code == "PR-A-1") {
                target = "3231";
            }
            else if (code == "PR-C-2") {
                target = "3212";
            }
            else {
                continue;
            }
            std::vector<std::string> route_drops;
            for (const auto& drop : entry.get("dropInfos", json::array {})) {
                if (drop.get("dropType", "") != "FURNITURE") {
                    route_drops.emplace_back(drop.get("itemId", ""));
                }
            }
            assert(route_supported(route_evidence(code, id, cost, route_drops, target)));
            assert(!route_supported(route_evidence(code, id, cost, route_drops, "30135")));
            ++checked;
        }
        assert(checked == 7);
        std::cout << "PASS current native stage route catalog: " << checked << " representative routes\n";
    }
    if (argc >= 4) {
        // Offline replay of archived native callbacks is not a new game run.
        const auto events = json::open(argv[3]);
        assert(events && events->is_array());
        Inventory archived;
        bool fresh_depot_seen = false;
        int results_seen = 0;
        for (const auto& event : events->as_array()) {
            if (event.get("what", "") == "DepotInfo" && event.get("details", "done", false)) {
                const auto counts = depot_counts(event.get("details", "verified_data", ""));
                assert(counts && archived.replace_fresh(*counts, true, true));
                fresh_depot_seen = true;
            }
            else if (event.get("what", "") == "StageDrops") {
                assert(fresh_depot_seen);
                const auto before_rock = archived.count("30012");
                const auto before_record = archived.count("2001");
                assert(before_rock && before_record);
                assert(apply_actual_stage_drops(archived, event, "1-7", "main_01-07"));
                assert(archived.count("30012") == *before_rock + 1);
                assert(archived.count("2001") == *before_record + 1);
                const auto after = archived.count("30012");
                assert(!apply_actual_stage_drops(archived, event, "LS-6", "wk_kc_6"));
                assert(archived.count("30012") == after);
                ++results_seen;
            }
        }
        assert(results_seen == 1);
        std::cout << "PASS archived native result compatibility replay (not a new game test)\n";
    }
    std::cout << "PASS inventory, atomic drops, fallback budget and reserve dependency assertions\n";
}
