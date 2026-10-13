#include "DepotMaintainTask.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/Miscellaneous/StageDropsConfig.h"
#include "DepotTask.h"
#include "FightTask.h"
#include "Task/Miscellaneous/DepotMaintainExecution.h"
#include "Task/Miscellaneous/DepotMaintainRecipeWire.h"
#include "Task/Miscellaneous/DepotMaintainRoutes.h"
#include "Task/Miscellaneous/DepotMaintainWire.h"
#include <climits>
#include <mutex>

asst::DepotMaintainTask::DepotMaintainTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
}

bool asst::DepotMaintainTask::set_params(const json::value& params)
{
    // Immutable once started. In particular, no late GUI callback may replace
    // a worker's fresh inventory decision with cached information.
    if (m_running || !depot_maintain::valid_params(params)) {
        return false;
    }
    auto plans = params.find<json::array>("plans");
    if (!plans || plans->size() > 256) {
        return false;
    }
    std::vector<Plan> parsed;
    for (const auto& entry : *plans) {
        if (!entry.is_object()) {
            return false;
        }
        auto target = depot_maintain::recognized_count(entry.get("target", json::value(0)));
        if (!target || *target > INT_MAX) {
            return false;
        }
        Plan plan { entry.get("item_id", ""),   static_cast<int>(*target),           entry.get("stage", ""),
                    entry.get("enabled", true), entry.get("stage_open", false),      entry.get("medicine", 0),
                    entry.get("stone", 0),      entry.get("medicine_expire_days", 0) };
        if (plan.enabled && (plan.item.empty() || plan.target <= 0 ||
                             (!params.get("dependency_preview", false) && plan.stage.empty()))) {
            return false;
        }
        parsed.emplace_back(std::move(plan));
    }
    if (auto options = params.find_value("fight_options"); options && !options->is_object()) {
        return false;
    }
    m_plans = std::move(parsed);
    m_fight_options = params.get("fight_options", json::value(json::object {}));
    // Shared fight options never authorize recovery. Only explicit, validated
    // per-plan budgets below are passed to that plan's single FightTask.
    m_fight_options["medicine"] = 0;
    m_fight_options["stone"] = 0;
    m_fight_options["medicine_expire_days"] = 0;
    m_fight_options["times"] = INT_MAX;
    m_fight_options["series"] = 1;
    m_fallback_stage = params.get("fallback_stage", "");
    m_fallback_open = params.get("fallback_open", false);
    m_only_first = params.get("only_first", false);
    m_dependency_preview = params.get("dependency_preview", false);
    return true;
}

void asst::DepotMaintainTask::report(const std::string& status, const json::value& data)
{
    auto info = basic_info_with_what("DepotMaintainInfo");
    info["details"] = data;
    info["details"]["status"] = status;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::DepotMaintainTask::run()
{
    if (!m_enable) {
        return true;
    }
    m_running = true;

    struct State
    {
        std::mutex mutex;
        depot_maintain::Inventory inventory;
        depot_maintain::Inventory::Counts recognized;
        bool done = false;
        bool invalid = false;
        depot_maintain::RecoveryStopEvidence recovery_stop;
        std::string expected_stage;
        std::string expected_stage_id;
    };

    // Callbacks from report upload threads may outlive a nested FightTask.
    // Capture shared state and the external callback by value, never `this`.
    auto state = std::make_shared<State>();
    const auto forward = m_callback;
    const auto id = m_task_id;
    auto nested_callback = [state, forward, id](AsstMsg msg, const json::value& details, Assistant* inst) {
        std::optional<json::object> verified_inventory;
        {
            std::lock_guard lock(state->mutex);
            state->recovery_stop.observe(details, msg == AsstMsg::SubTaskCompleted, msg == AsstMsg::SubTaskExtraInfo);
        }
        if (msg == AsstMsg::SubTaskError && depot_maintain::is_drop_recognition_task(details)) {
            // Plugin errors do not propagate through FightTask::run itself.
            // A later successful drop cannot erase an unobserved earlier one.
            std::lock_guard lock(state->mutex);
            state->invalid = true;
        }
        if (msg == AsstMsg::SubTaskExtraInfo) {
            std::lock_guard lock(state->mutex);
            const auto what = details.get("what", "");
            if (what == "DepotInfo") {
                state->invalid |= !depot_maintain::consume_depot_snapshot(state->recognized, state->done, details);
            }
            else if (what == "StageDrops") {
                verified_inventory = depot_maintain::accept_stage_inventory(
                    state->inventory,
                    details,
                    state->expected_stage,
                    state->expected_stage_id,
                    state->invalid);
            }
        }
        if (forward) {
            auto scoped = details;
            scoped["taskchain"] = std::string(TaskType);
            scoped["taskid"] = id;
            if (verified_inventory) {
                scoped["details"]["verified_inventory"] = std::move(*verified_inventory);
            }
            forward(msg, scoped, inst);
        }
    };

    auto allow_next_battle = [state]() {
        std::lock_guard lock(state->mutex);
        return !state->invalid;
    };

    report("recognizing_fresh_depot");
    DepotTask depot(nested_callback, m_inst);
    depot.set_task_id(m_task_id);
    depot.set_strict_scan(true);
    const bool depot_ok = depot.run();
    {
        std::lock_guard lock(state->mutex);
        if (!state->inventory.replace_fresh(state->recognized, depot_ok && !state->invalid, state->done)) {
            report("depot_unverified_no_farming");
            return false;
        }
        report(
            "depot_verified",
            json::object { { "recognized_items", state->recognized.size() },
                           { "source", "fresh_depot_verified_data" } });
    }

    if (m_dependency_preview) {
        // Isolated fresh-depot preview. Never enter FightTask, fallback or
        // synthesis: a feasible material graph is not a real craft receipt.
        const auto recipes = depot_maintain::recipe_catalog(ItemData.get_item_formulas(), ItemData.get_all_item_id());
        if (!recipes) {
            report("recipe_catalog_unverified_no_farming");
            return false;
        }
        std::vector<depot_maintain::Goal> goals;
        std::set<std::string> direct;
        for (const auto& plan : m_plans) {
            goals.push_back({ plan.item, plan.target, plan.enabled, plan.stage_open });
            // A selected stage opts this item into direct acquisition even
            // when it also has a native workshop recipe.
            if (plan.enabled && !plan.stage.empty()) {
                direct.emplace(plan.item);
            }
        }
        std::lock_guard lock(state->mutex);
        const auto preview = depot_maintain::preview_dependencies(state->inventory, *recipes, goals, direct);
        report(
            preview.verified() ? "dependency_preview_ready_not_targets_met" : "dependency_preview_unverified",
            depot_maintain::dependency_receipt(preview));
        return preview.verified();
    }

    int executed = 0;
    std::vector<bool> route_blocked(m_plans.size(), false);
    for (size_t index = 0; index < m_plans.size(); ++index) {
        if (need_exit()) {
            return false;
        }
        const auto& plan = m_plans[index];
        if (!plan.enabled) {
            continue;
        }
        std::optional<int64_t> need;
        std::optional<int64_t> observed_count;
        {
            std::lock_guard lock(state->mutex);
            need = state->inventory.shortage(plan.item, plan.target);
            observed_count = state->inventory.count(plan.item);
        }
        json::value data = json::object { { "index", index },
                                          { "item_id", plan.item },
                                          { "target", plan.target },
                                          { "stage", plan.stage } };
        if (!need || !observed_count) {
            report("inventory_unknown", data);
            continue;
        }
        data["shortage"] = *need;
        // Use the exact count from the same locked snapshot, not target minus
        // clamped shortage (which loses stock above target).
        data["count_before"] = *observed_count;
        if (*need == 0) {
            report("target_met", data);
            continue;
        }
        if (!plan.stage_open) {
            report("stage_closed", data);
            continue;
        }

        const auto& stage_info = StageDrops.get_stage_info(plan.stage, StageDifficulty::Normal);
        std::vector<std::string> listed_items;
        for (const auto& [type, items] : stage_info.drops) {
            if (type == StageDropType::Normal || type == StageDropType::Extra || type == StageDropType::Special) {
                listed_items.insert(listed_items.end(), items.begin(), items.end());
            }
        }
        const auto route = depot_maintain::route_evidence(
            plan.stage,
            stage_info.stage_id,
            stage_info.ap_cost,
            listed_items,
            plan.item);
        if (!depot_maintain::route_supported(route)) {
            route_blocked[index] = true;
            data["stage_id"] = stage_info.stage_id;
            report(
                route == depot_maintain::RouteEvidence::StageUnknown ? "stage_unknown_no_farming"
                                                                     : "stage_item_not_listed_no_farming",
                data);
            continue;
        }
        auto fight_params = m_fight_options;
        fight_params["stage"] = plan.stage;
        fight_params["drops"] = json::object { { plan.item, *need } };
        fight_params["medicine"] = plan.medicine;
        fight_params["stone"] = plan.stone;
        fight_params["medicine_expire_days"] = plan.medicine_expire_days;
        // Exactly one native FightTask per configured plan. Its plugin and
        // ProcessTask counters survive internal retries; never reconstruct it
        // to retry an incomplete shortage and re-arm a consumed allowance.
        FightTask fight(nested_callback, m_inst);
        fight.set_task_id(m_task_id);
        fight.set_drop_guard(allow_next_battle);
        {
            std::lock_guard lock(state->mutex);
            state->recovery_stop = {};
            state->expected_stage = plan.stage;
            state->expected_stage_id = stage_info.stage_id;
        }
        report("farming_shortage", data);
        report(
            "recovery_policy",
            json::object { { "index", index },
                           { "medicine", plan.medicine },
                           { "stone", plan.stone },
                           { "medicine_expire_days", plan.medicine_expire_days } });
        const bool fight_ok = fight.set_params(fight_params) && fight.run();
        // Always report consumption even when a later battle or recognition
        // fails. Stone clicks are not receipts; leave that quantity unknown.
        report(
            "recovery_receipt",
            json::object { { "index", index },
                           { "native_confirmed_medicine_count", fight.get_confirmed_medicine_count() },
                           { "medicine_source", "MedicineCounterTaskPlugin" },
                           { "stone_consumed", json::value() },
                           { "stone_source", "unverified" } });
        if (!fight_ok || need_exit()) {
            report("fight_failed_or_stopped", data);
            return false;
        }
        ++executed;
        bool sanity_blocked = false;
        {
            std::lock_guard lock(state->mutex);
            sanity_blocked = state->recovery_stop.verified_stop();
            need = state->inventory.shortage(plan.item, plan.target);
            observed_count = state->inventory.count(plan.item);
            if (state->invalid || !need || !observed_count) {
                report("drops_unverified", data);
                return false;
            }
        }
        data["shortage"] = *need;
        data["count_after"] = *observed_count;
        const auto decision = depot_maintain::shortage_fight_decision(*need, sanity_blocked);
        if (decision != depot_maintain::ShortageFightDecision::TargetMet) {
            // Do not infer a refill, successful quota, or extra farming from
            // FightTask merely returning. Natural sanity may be insufficient.
            if (decision != depot_maintain::ShortageFightDecision::NaturalBudgetStop) {
                report("incomplete_unverified_stop", data);
                return false;
            }
            const bool natural_only = plan.medicine == 0 && plan.stone == 0 && plan.medicine_expire_days == 0;
            report(natural_only ? "natural_sanity_insufficient" : "authorized_sanity_insufficient", data);
            // The first verified natural-budget shortage ends this owned
            // task. Do not proceed to a cheaper/lower-priority plan or fallback.
            report(
                natural_only ? "natural_budget_finished_targets_incomplete"
                             : "authorized_budget_finished_targets_incomplete",
                data);
            return true;
        }
        report("target_met", data);
        if (m_only_first && executed > 0) {
            report("only_first_stop");
            return true;
        }
    }
    std::vector<depot_maintain::Goal> goals;
    goals.reserve(m_plans.size());
    for (size_t index = 0; index < m_plans.size(); ++index) {
        const auto& plan = m_plans[index];
        // Actual later co-drops may satisfy an earlier blocked route normally.
        goals.push_back({ plan.item, plan.target, plan.enabled, plan.stage_open && !route_blocked[index] });
    }
    depot_maintain::Completion final_state;
    {
        std::lock_guard lock(state->mutex);
        final_state = depot_maintain::completion(state->inventory, goals, false);
    }
    if (final_state == depot_maintain::Completion::Unresolved) {
        report("unresolved_targets_no_fallback");
        return false;
    }
    if (final_state == depot_maintain::Completion::UnverifiedStop) {
        report("incomplete_unverified_stop");
        return false;
    }
    report("all_targets_met");
    if (!m_fallback_stage.empty()) {
        if (!m_fallback_open) {
            report("fallback_closed");
            return false;
        }
        const auto& stage_info = StageDrops.get_stage_info(m_fallback_stage, StageDifficulty::Normal);
        if (!depot_maintain::known_natural_stage(stage_info.stage_id, stage_info.ap_cost)) {
            report("fallback_stage_unknown_no_farming", json::object { { "stage", m_fallback_stage } });
            return false;
        }
        auto params = m_fight_options;
        params["stage"] = m_fallback_stage;
        params["drops"] = json::object {};
        FightTask fallback(nested_callback, m_inst);
        fallback.set_task_id(m_task_id);
        fallback.set_drop_guard(allow_next_battle);
        {
            std::lock_guard lock(state->mutex);
            state->recovery_stop = {};
            state->expected_stage = m_fallback_stage;
            state->expected_stage_id = stage_info.stage_id;
        }
        report("farming_fallback", json::object { { "stage", m_fallback_stage } });
        if (!fallback.set_params(params) || !fallback.run() || need_exit()) {
            report("fallback_failed_or_stopped");
            return false;
        }
        {
            std::lock_guard lock(state->mutex);
            if (state->invalid) {
                report("fallback_drops_unverified");
                return false;
            }
            if (!depot_maintain::fallback_budget_verified(!state->invalid, state->recovery_stop.verified_stop())) {
                report("fallback_incomplete_unverified_stop");
                return false;
            }
        }
        report("fallback_natural_budget_finished");
    }
    report("finished");
    return true;
}
