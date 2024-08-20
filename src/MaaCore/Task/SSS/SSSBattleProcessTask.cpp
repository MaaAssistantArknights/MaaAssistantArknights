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
    ranges::transform(m_sss_combat_data.strategies, std::inserter(m_all_cores, m_all_cores.begin()),
                      [](const auto& strategy) { return strategy.core; });
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
            m_cur_deployment_opers, old_deployment_opers,
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
    if (const auto now = std::chrono::steady_clock::now();
        prev_frame_time > now - min_frame_interval) [[unlikely]] {
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
        if (ranges::count_if(m_cur_deployment_opers,
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

    std::unordered_map<std::string, DeploymentOper> exist_core;
    std::vector<DeploymentOper> tool_men;
    for (const auto& oper : m_cur_deployment_opers) {
        if (m_all_cores.contains(oper.name)) {
            exist_core.emplace(oper.name, oper);
        }
        else if (oper.is_unusual_location && !m_all_action_opers.contains(oper.name)) {
            tool_men.emplace_back(oper);
            // 工具人的技能一概好了就用
            m_skill_usage.try_emplace(oper.name, SkillUsage::Possibly);
        }
    }

    for (auto& strategy : m_sss_combat_data.strategies) {
        // 步骤(strategy)间锁，以部署位置为key，保证core不被顶替
        auto it = m_sss_combat_data.order.find(strategy.location);
        if (*(it->second.front()) != strategy) {
            continue;
        }

        bool use_the_core = ranges::all_of(strategy.tool_men, [](const auto& pair) { return pair.second <= 0; }) &&
                            !strategy.core.empty() && exist_core.contains(strategy.core);
        if (use_the_core) {
            const auto& core = exist_core.at(strategy.core);
            // 全局保留core以备暴毙等情况
            if (strategy.core_deployed && core.role != Role::Drone && core.role != Role::Unknown) {
                strategy.core_deployed = false;
                for (auto& strategy_reset : m_sss_combat_data.strategies) {
                    if (strategy.location == strategy_reset.location) {
                        for (auto& same_location_strategy : (it->second)) {
                            if ((*same_location_strategy).index == strategy_reset.index) {
                                bool skip = false;
                                for (auto& [role, quantity] : (*same_location_strategy).tool_men) {
                                    if (role == Role::Drone || role == Role::Unknown) {
                                        skip = true;
                                    }
                                }
                                if (skip) {
                                    continue;
                                }
                                strategy_reset = *same_location_strategy;
                                break;
                            }
                        }
                    }
                }
                (it->second).emplace_back((it->second).front());
                (it->second).erase((it->second).begin());
                return false;
            }

            if (!core.available) {
                // 直接返回，等费用，等下次循环处理部署逻辑
                break;
            }
            // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
            return deploy_oper(strategy.core, strategy.location, strategy.direction) && update_deployment();
        }

        bool skip = false;
        for (auto& [role, quantity] : strategy.tool_men) {
            if (quantity <= 0) {
                continue;
            }
            // for apple-clang build error
            Role role_for_lambda = role;

            // 如果有可用的干员，直接使用
            auto available_iter = ranges::find_if(tool_men, [&](const DeploymentOper& oper) {
                return oper.available && oper.role == role_for_lambda && !m_all_cores.contains(oper.name);
            });
            if (available_iter != tool_men.cend()) {
                --quantity;
                // 每个tool_men用尽之后删除m_sss_combat_data.strategies中tool_men的当前元素
                // 重新执行该location所有策略时用m_sss_combat_data.order进行恢复
                if (quantity <= 0) {
                    strategy.tool_men.erase(role);
                    if (strategy.tool_men.empty() && strategy.core.empty()) {
                        (it->second).emplace_back((it->second).front());
                        (it->second).erase((it->second).begin());
                    }
                }
                // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
                return deploy_oper(available_iter->name, strategy.location, strategy.direction) && update_deployment();
            }

            auto not_available_iter = ranges::find_if(tool_men, [&](const DeploymentOper& oper) {
                return oper.role == role_for_lambda && !m_all_cores.contains(oper.name);
            });
            if (not_available_iter == tool_men.cend()) {
                continue;
            }
            // 如果有对应职业干员，但费用没转好，就等他转好，而不是部署下一个策略中的 tool_men
            // 直接返回出去，后续逻辑交给下次循环处理
            skip = true;
            break;
        }
        if (skip) {
            break;
        }
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
