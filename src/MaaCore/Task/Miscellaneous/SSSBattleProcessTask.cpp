#include "SSSBattleProcessTask.h"

#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

using namespace asst::battle;
using namespace asst::battle::sss;

bool asst::SSSBattleProcessTask::set_stage_name(const std::string& stage_name)
{
    LogTraceFunction;

    if (!BattleHelper::set_stage_name(stage_name)) {
        json::value info = basic_info_with_what("UnsupportedLevel");
        auto& details = info["details"];
        details["level"] = m_stage_name;
        callback(AsstMsg::SubTaskExtraInfo, info);

        return false;
    }
    if (m_stage_index >= SSSCopilot.stages_size()) {
        Log.error("SSS CopilotConfig: stage index out of range", m_stage_index);
        return false;
    }

    m_sss_combat_data = SSSCopilot.get_data(m_stage_index);
    ranges::transform(m_sss_combat_data.strategies, std::inserter(m_all_cores, m_all_cores.begin()),
                      [](const auto& strategy) { return strategy.core; });

    return true;
}

bool asst::SSSBattleProcessTask::set_stage_index(size_t index)
{
    Log.info(__FUNCTION__, index);

    if (index >= SSSCopilot.stages_size()) {
        return false;
    }
    m_stage_index = index;
    return true;
}

bool asst::SSSBattleProcessTask::do_derived_action(size_t action_index)
{
    LogTraceFunction;

    const auto& action = get_combat_data().actions.at(action_index);

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
    update_deployment(false, image);

    std::unordered_map<std::string, DeploymentOper> exist_core;
    std::unordered_map<Role, DeploymentOper> available_tool_men;
    for (const auto& [name, oper] : m_cur_deployment_opers) {
        if (m_all_cores.contains(name)) {
            exist_core.emplace(name, oper);
        }
        else if (oper.available) {
            available_tool_men.emplace(oper.role, oper);
        }
    }

    for (auto& strategy : m_sss_combat_data.strategies) {
        bool use_the_core = ranges::all_of(strategy.tool_men, [](const auto& pair) { return pair.second <= 0; }) &&
                            !strategy.core.empty() && exist_core.contains(strategy.core);
        if (use_the_core) {
            const auto& core = exist_core.at(strategy.core);
            if (!core.available) {
                // 直接返回，等费用，等下次循环处理部署逻辑
                return true;
            }
            // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
            return deploy_oper(strategy.core, strategy.location, strategy.direction);
        }

        for (auto& [role, quantity] : strategy.tool_men) {
            if (quantity <= 0) {
                continue;
            }
            // for apple-clang build error
            Role role_for_lambda = role;
            auto iter = ranges::find_if(available_tool_men,
                                        [role_for_lambda](const auto& pair) { return pair.first == role_for_lambda; });
            if (iter == available_tool_men.end()) {
                continue;
            }

            --quantity;
            // 部署完，画面会发生变化，所以直接返回，后续逻辑交给下次循环处理
            return deploy_oper(iter->second.name, strategy.location, strategy.direction);
        }
    }

    if (m_sss_combat_data.draw_as_possible) {
        draw_card();
    }

    use_all_ready_skill(image);

    return true;
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
            if (cur_counts[role] < number) {
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

bool asst::SSSBattleProcessTask::draw_card()
{
    return ProcessTask(*this, { "SSSDrawCard" }).run();
}
