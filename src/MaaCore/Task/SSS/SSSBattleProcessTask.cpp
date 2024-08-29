#include "SSSBattleProcessTask.h"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

using namespace asst::battle;
using namespace asst::battle::sss;

bool asst::SSSBattleProcessTask::set_stage_name(const std::string& stage_name)
{
    Log.info(__FUNCTION__, stage_name);

    if (!SSSCopilot.contains(stage_name)) {
        Log.error("SSS SSSBattleProcessTask: unknown name", stage_name);
        return false;
    }
    m_sss_combat_data = SSSCopilot.get_data(stage_name);
    ranges::transform(
        m_sss_combat_data.strategies | views::filter([](const auto& strategy) { return strategy.core.has_value(); }),
        std::inserter(m_all_cores, m_all_cores.begin()),
        [](const auto& strategy) { return strategy.core.value(); });
    for (const auto& action : m_sss_combat_data.actions) {
        if (action.type == battle::copilot::ActionType::Deploy) {
            m_all_action_opers.emplace(action.name);
        }
    }

    if (!BattleHelper::set_stage_name(m_sss_combat_data.info.stage_name)) {
        json::value info = basic_info_with_what("UnsupportedLevel");
        auto& details = info["details"];
        details["level"] = m_sss_combat_data.info.stage_name;
        callback(AsstMsg::SubTaskExtraInfo, info);

        return false;
    }
    return true;
}

bool asst::SSSBattleProcessTask::update_deployment_with_skip(const cv::Mat& reusable)
{
    const auto now = std::chrono::steady_clock::now();
    const std::vector<DeploymentOper> old_deployment_opers = m_cur_deployment_opers;
    static auto last_same_time = now;
    static auto last_skip_time = now;
    static auto interval_time = 0;

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_skip_time).count() < interval_time) {
        Log.trace("Passed without update deployment");
        sleep(interval_time);
        return true;
    }
    last_skip_time = now;

    if (!update_deployment(false, reusable)) {
        return false;
    }

    if (ranges::equal(
            m_cur_deployment_opers,
            old_deployment_opers,
            [](const DeploymentOper& oper1, const DeploymentOper& oper2) { return oper1.name == oper2.name; })) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_same_time).count() > 30000) {
            // 30s 能回 60 费，基本上已经到了挂机的时候，放缓检查的速度
            Log.trace("30s is unchanged and the waiting time is extended to 1s");
            interval_time = 1000;
        }
    }
    else {
        last_same_time = now;
        Log.trace("Changed, the waiting time is reset to 0s");
        interval_time = 0;
    }

    return true;
}

bool asst::SSSBattleProcessTask::do_derived_action(const battle::copilot::Action& action, size_t index)
{
    LogTraceFunction;

    std::ignore = index;

    switch (action.type) {
    case battle::copilot::ActionType::DrawCard:
        return draw_card() && update_deployment();
    case battle::copilot::ActionType::CheckIfStartOver:
        return check_if_start_over(action);
    default:
        Log.error("unknown action type", static_cast<int>(action.type));
        return false;
    }
}

bool asst::SSSBattleProcessTask::do_strategic_action(const cv::Mat& reusable)
{
    LogTraceFunction;

    thread_local auto prev_frame_time = std::chrono::steady_clock::time_point {};
    static const auto min_frame_interval = std::chrono::milliseconds(Config.get_options().sss_fight_screencap_interval);

    // prevent our program from consuming too much CPU
    if (const auto now = std::chrono::steady_clock::now(); prev_frame_time > now - min_frame_interval) [[unlikely]] {
        Log.debug("Sleeping for framerate limit");
        std::this_thread::sleep_for(min_frame_interval - (now - prev_frame_time));
    }

    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;
    prev_frame_time = std::chrono::steady_clock::now();

    if (check_and_get_drops(image)) {
        sleep(1000);
        return true;
    }

    if (check_and_do_strategy(image)) {
        image = ctrler()->get_image();
    }

    if (use_all_ready_skill(image)) {
        // image = ctrler()->get_image();
    }

    if (m_sss_combat_data.draw_as_possible && draw_card(false, image)) {
        // image = ctrler()->get_image();
    }

    return true;
}

bool asst::SSSBattleProcessTask::wait_until_start(bool weak)
{
    LogTraceFunction;
    if (!ProcessTask(*this, { "SSSFightStart-PreSelect-Match" }).set_retry_times(300).run()) {
        return false;
    }
    update_deployment();
    ProcessTask(*this, { "SSSFightStart-PreSelect-Clear" }).run();

    int replace_count;     // 替换干员数量，装置不计数
    int replace_limit = 4; // 替换数量限制，最多替换4个
    int cost_limit = 29;   // 费用阈值，低于该费用的干员不替换
    // 暂时写死凑合一下
    if (m_all_action_opers.contains("超重绝缘水泥") || m_all_cores.contains("超重绝缘水泥")) {
        replace_count = 1; // 只换水泥+1个，以防换出水泥
    }
    else {
        replace_count = 4;
        if (ranges::count_if(
                m_cur_deployment_opers,
                [](const auto& oper) { return oper.role == Role::Pioneer; }) /* 先锋数量 */
            < 2) {
            cost_limit = 25; // 先锋低于2个时，降低费用阈值，以试图换出先锋
        }
    }
    for (const auto& oper : m_cur_deployment_opers | views::reverse) {
        if (replace_limit <= 0 || oper.cost < cost_limit) {
            break;
        }
        if (oper.role == Role::Drone) {
            // 直接抛弃水泥
            ctrler()->click(oper.rect);
            Log.info(__FUNCTION__, "replace Drone, name:", oper.name);
            --replace_limit;
        }
        else if (replace_count > 0 && !m_all_cores.contains(oper.name)) {
            ctrler()->click(oper.rect);
            Log.info(__FUNCTION__, "replace oper, name:", oper.name);
            --replace_count;
            --replace_limit;
        }
    }
    return ProcessTask(*this, { "SSSFightStart-PreSelect", "SSSFightStart-PreSelect-Confirm" }).run() &&
           BattleProcessTask::wait_until_start(weak);
}

bool asst::SSSBattleProcessTask::check_and_do_strategy(const cv::Mat& reusable)
{
    if (m_all_cores.empty()) {
        return false;
    }

    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;
    if (!update_deployment_with_skip(image)) {
        return false;
    }

    std::unordered_map<std::string, DeploymentOper> exist_core; // 在待部署区的 core
    std::vector<DeploymentOper> tool_men;
    for (const auto& oper : m_cur_deployment_opers) {
        if (m_all_cores.contains(oper.name)) {
            exist_core.emplace(oper.name, oper);
        }
        else if (oper.is_usual_location && !m_all_action_opers.contains(oper.name)) {
            tool_men.emplace_back(oper);
            // 工具人的技能一概好了就用
            m_skill_usage.try_emplace(oper.name, SkillUsage::Possibly);
        }
    }

    auto tool_men_done = [&](const RoleCounts& tool_men) -> bool {
        return ranges::all_of(tool_men | views::values, [](int counts) { return counts <= 0; });
    };

    /* 不再检查干员是否暴毙
    auto plain_oper = [&](Role role) -> bool {
        return role != Role::Drone && role != Role::Unknown;
    };
    for (Strategy& strategy :
         m_sss_combat_data.loc_stragegies | views::values | views::transform([&](const auto& locs) -> Strategy& {
             return m_sss_combat_data.strategies[locs.back()]; // 仅检查同格子最靠后的 strategy 的部署情况
         })) {
        if (strategy.core_deployed &&                          // 当前 strategy 已经完毕
            !strategy.core.empty() &&                          // 存在 core
            exist_core.contains(strategy.core) &&              // core 在待部署区
            plain_oper(exist_core.at(strategy.core).role)) {   // core 是干员
            // 已经部署过的干员 core 出现在待部署区，可能是暴毙，重置当前格子的所有 strategies 信息
            LogWarn << __FUNCTION__ << "| Core" << strategy.core
                    << "is already deployed, reset all strategies in location" << strategy.location;
            for (int x : m_sss_combat_data.loc_stragegies[strategy.location]) {
                auto& strategy_reset = m_sss_combat_data.strategies[x];
                strategy_reset.core_deployed = false;
                strategy_reset.required_tool_men = strategy_reset.origin_tool_men;
            }
        }
    }
    */

    // 同格子是否已经存在顺位靠前但未执行完毕的 strategy
    std::unordered_set<asst::Point> loc_with_strategy;
    for (auto& strategy : m_sss_combat_data.strategies) {
        if (strategy.all_deployed) {
            // 跳过已经执行完毕的 strategy
            continue;
        }
        if (loc_with_strategy.contains(strategy.location)) {
            // 跳过同格子存在未执行完毕的 strategy
            continue;
        }
#ifdef ASST_DEBUG
        LogDebug << __FUNCTION__ << "| Checking strategy at" << strategy.location << "with core"
                 << strategy.core.value_or("(empty)")
                 << "and tool_men"
                 << (strategy.tool_men | views::transform([](const auto& rolecounts) {
                         return asst::enum_to_string(rolecounts.first) + ": " + std::to_string(rolecounts.second);
                     }));
#endif
        bool use_the_core =
            strategy.core.has_value() && exist_core.contains(strategy.core.value()) && tool_men_done(strategy.tool_men);
        if (use_the_core) {
            const auto& core = exist_core.at(strategy.core.value());
            if (!core.available) {
                Log.trace(__FUNCTION__, "| Core", core.name, "is not available, waiting");
                // 直接返回，等费用，等下次循环处理部署逻辑
                return false;
            }
            strategy.all_deployed = true;
            strategy.core.reset();
            Log.info(__FUNCTION__, "| Deploy core", core.name, "at", strategy.location);

            // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
            if (auto it = m_all_cores.find(core.name); it != m_all_cores.end()) {
                m_all_cores.erase(it);
            }
            else {
                Log.error(__FUNCTION__, "| Core", core.name, " in strategy, but not found in all_cores");
            }

            return deploy_oper(core.name, strategy.location, strategy.direction) && update_deployment();
        }

        auto required_roles_view = strategy.tool_men |
                                   views::filter([](const auto& tool_man) { return tool_man.second > 0; }) |
                                   views::keys;
        auto required_roles = std::unordered_set(required_roles_view.begin(), required_roles_view.end());

        // 如果有费用转好的干员，直接使用
        if (auto available_iter = ranges::find_if(
                tool_men,
                [&](const DeploymentOper& oper) {
                    return required_roles.contains(oper.role) && // 职业匹配
                           oper.available &&                     // 干员费用已经转好
                           !m_all_cores.contains(oper.name);     // 避免把 core 当作工具人部署
                });
            available_iter != tool_men.end()) {
            --strategy.tool_men[available_iter->role];

            if (!strategy.core.has_value() && tool_men_done(strategy.tool_men)) {
                // 如果没有 core，且所有工具人都用完了，就直接算执行完毕
                strategy.all_deployed = true;
            }
            Log.info(__FUNCTION__, "| Deploy tool_man", available_iter->name, "at", strategy.location);

            // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
            return deploy_oper(available_iter->name, strategy.location, strategy.direction) && update_deployment();
        }

        if (ranges::any_of(tool_men, [&](const auto& oper) {
                return required_roles.contains(oper.role) && !m_all_cores.contains(oper.name);
            })) {
            // 如果待部署区有符合要求的干员，但是费用还没转好，就等待
            return false;
        }

        // 若有 core，则没有执行完毕时忽略同一格后续策略
        if (strategy.core.has_value()) {
            loc_with_strategy.emplace(strategy.location);
        }
        // 若没有 core，则允许在待部署区没有所需工具人时（不论费用是否转好），允许继续检查同一格后续策略
        // else { continue; }

        // 如果在此 strategy 没有部署干员，而且不是在等待 core 部署，就尝试下一个 strategy
        // continue;
    }

    return false;
}

bool asst::SSSBattleProcessTask::check_if_start_over(const battle::copilot::Action& action)
{
    LogTraceFunction;

    update_deployment();

    bool to_abandon = false;

    if (!action.name.empty() &&
        !ranges::any_of(m_cur_deployment_opers, [&](const auto& oper) { return oper.name == action.name; }) &&
        !m_battlefield_opers.contains(action.name)) {
        to_abandon = true;
    }
    else if (!action.role_counts.empty()) {
        std::unordered_map<Role, size_t> cur_counts;
        for (const auto& oper : m_cur_deployment_opers) {
            cur_counts[oper.role] += 1;
        }
        for (const auto& [role, number] : action.role_counts) {
            if (cur_counts[role] < static_cast<size_t>(number)) {
                to_abandon = true;
                break;
            }
        }
    }

    if (to_abandon) {
        abandon();
    }

    return true;
}

bool asst::SSSBattleProcessTask::draw_card(bool with_retry, const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;

    ProcessTask task(*this, { "SSSDrawCard" });
    if (!with_retry) {
        task.set_task_delay(0).set_retry_times(0);
    }
    task.set_reusable_image(image);
    return task.run();
}

bool asst::SSSBattleProcessTask::check_and_get_drops(const cv::Mat& reusable)
{
    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;
    if (!ProcessTask(*this, { "SSSHalfTimeDropsBegin" })
             .set_reusable_image(image)
             .set_times_limit("SSSHalfTimeDropsBegin", 0)
             .set_task_delay(0)
             .set_retry_times(0)
             .run()) {
        return false;
    }

    const auto& drops = m_sss_combat_data.order_of_drops;
    std::string task_name;
    if (drops.empty()) {
        task_name = inst_string() + "@SSSHalfTimeDropsCancel";
    }
    else {
        Task.get<OcrTaskInfo>(inst_string() + "@SSSHalfTimeDrops")->text = { drops };
        task_name = inst_string() + "@SSSHalfTimeDropsBegin";
    }
    Log.info("Get drops", drops);
    return ProcessTask(*this, { task_name }).set_reusable_image(image).set_retry_times(3).run();
}
