#include "SSSBattleProcessTask.h"

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
        details["level"] = m_stage_name;
        callback(AsstMsg::SubTaskExtraInfo, info);

        return false;
    }
    return true;
}

bool asst::SSSBattleProcessTask::do_derived_action(const battle::copilot::Action& action, size_t index)
{
    LogTraceFunction;

    std::ignore = index;

    switch (action.type) {
    case battle::copilot::ActionType::DrawCard:
        return draw_card();
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
    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;

    if (check_and_get_drops(image)) {
        return true;
    }

    if (m_sss_combat_data.draw_as_possible && draw_card(false, image)) {
        image = ctrler()->get_image();
    }

    if (check_and_do_strategy(image)) {
        image = ctrler()->get_image();
    }

    if (use_all_ready_skill(image)) {
        // image = ctrler()->get_image();
    }

    return true;
}

bool asst::SSSBattleProcessTask::wait_until_start(bool weak)
{
    LogTraceFunction;

    return ProcessTask(*this, { "SSSFightDirectly" }).set_retry_times(300).run() &&
           BattleProcessTask::wait_until_start(weak);
}

bool asst::SSSBattleProcessTask::check_and_do_strategy(const cv::Mat& reusable)
{
    if (m_all_cores.empty()) {
        return false;
    }

    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;
    if (!update_deployment(false, image)) {
        return false;
    }

    std::unordered_map<std::string, DeploymentOper> exist_core;
    std::vector<DeploymentOper> tool_men;
    for (const auto& [name, oper] : m_cur_deployment_opers) {
        if (m_all_cores.contains(name)) {
            exist_core.emplace(name, oper);
        }
        else if (oper.is_unusual_location && !m_all_action_opers.contains(oper.name)) {
            tool_men.emplace_back(oper);
            // 工具人的技能一概好了就用
            m_skill_usage.try_emplace(name, SkillUsage::Possibly);
        }
    }

    for (auto& strategy : m_sss_combat_data.strategies) {
        bool use_the_core = ranges::all_of(strategy.tool_men, [](const auto& pair) { return pair.second <= 0; }) &&
                            !strategy.core.empty() && exist_core.contains(strategy.core);
        if (use_the_core) {
            const auto& core = exist_core.at(strategy.core);
            if (!core.available) {
                // 直接返回，等费用，等下次循环处理部署逻辑
                break;
            }
            m_all_cores.erase(strategy.core);
            // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
            return deploy_oper(strategy.core, strategy.location, strategy.direction);
        }

        bool skip = false;
        for (auto& [role, quantity] : strategy.tool_men) {
            if (quantity <= 0) {
                continue;
            }
            // for apple-clang build error
            Role role_for_lambda = role;

            // 如果有可用的干员，直接使用
            auto available_iter = ranges::find_if(
                tool_men, [&](const DeploymentOper& oper) { return oper.available && oper.role == role_for_lambda; });
            if (available_iter != tool_men.cend()) {
                --quantity;
                // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
                return deploy_oper(available_iter->name, strategy.location, strategy.direction);
            }

            auto not_available_iter =
                ranges::find_if(tool_men, [&](const DeploymentOper& oper) { return oper.role == role_for_lambda; });
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
    if (!action.name.empty() && !m_cur_deployment_opers.contains(action.name) &&
        !m_battlefield_opers.contains(action.name)) {
        to_abandon = true;
    }
    else if (!action.role_counts.empty()) {
        std::unordered_map<Role, size_t> cur_counts;
        for (const auto& oper : m_cur_deployment_opers | views::values) {
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
    return ProcessTask(*this, { task_name }).set_reusable_image(image).run();
}
