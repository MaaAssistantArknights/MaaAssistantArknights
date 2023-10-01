#include "BattleProcessTask.h"

#include "Utils/Ranges.hpp"
#include <chrono>
#include <future>
#include <thread>

#include "Utils/NoWarningCV.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Algorithm.hpp"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

using namespace asst::battle;
using namespace asst::battle::copilot;

asst::BattleProcessTask::BattleProcessTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain)
    : AbstractTask(callback, inst, task_chain), BattleHelper(inst)
{}

bool asst::BattleProcessTask::_run()
{
    LogTraceFunction;
    clear();

    if (!calc_tiles_info(m_stage_name)) {
        Log.error("get stage info failed");
        return false;
    }

    update_deployment(true);
    to_group();

    size_t action_size = get_combat_data().actions.size();
    for (size_t i = 0; i < action_size && !need_exit() && m_in_battle; ++i) {
        const auto& action = get_combat_data().actions.at(i);
        do_action(action, i);
    }

    if (need_to_wait_until_end()) {
        wait_until_end();
    }

    return true;
}

void asst::BattleProcessTask::clear()
{
    BattleHelper::clear();

    m_oper_in_group.clear();
    m_in_bullet_time = false;
}

bool asst::BattleProcessTask::set_stage_name(const std::string& stage_name)
{
    LogTraceFunction;

    if (!BattleHelper::set_stage_name(stage_name)) {
        json::value info = basic_info_with_what("UnsupportedLevel");
        auto& details = info["details"];
        details["level"] = m_stage_name;
        callback(AsstMsg::SubTaskExtraInfo, info);

        return false;
    }

    m_combat_data = Copilot.get_data();

    return true;
}

void asst::BattleProcessTask::set_wait_until_end(bool wait_until_end)
{
    m_need_to_wait_until_end = wait_until_end;
}

bool asst::BattleProcessTask::to_group()
{
    std::unordered_map<std::string, std::vector<std::string>> groups;
    for (const auto& [group_name, oper_list] : get_combat_data().groups) {
        std::vector<std::string> oper_name_list;
        ranges::transform(oper_list, std::back_inserter(oper_name_list), [](const auto& oper) { return oper.name; });
        groups.emplace(group_name, std::move(oper_name_list));
    }

    std::unordered_set<std::string> char_set;
    for (const auto& oper : m_cur_deployment_opers | views::values) {
        char_set.emplace(oper.name);
    }

    auto result_opt = algorithm::get_char_allocation_for_each_group(groups, char_set);
    if (result_opt) {
        m_oper_in_group = *result_opt;
    }
    else {
        Log.warn("get_char_allocation_for_each_group failed");
        for (const auto& [gp, names] : groups) {
            if (names.empty()) {
                continue;
            }
            m_oper_in_group.emplace(gp, names.front());
        }
    }

    std::unordered_map<std::string, std::string> ungrouped;
    const auto& grouped_view = m_oper_in_group | views::values;
    for (const auto& name : char_set) {
        if (ranges::find(grouped_view, name) != grouped_view.end()) {
            continue;
        }
        ungrouped.emplace(name, name);
    }
    m_oper_in_group.merge(ungrouped);

    for (const auto& action : m_combat_data.actions) {
        const std::string& action_name = action.name;
        if (action_name.empty() || m_oper_in_group.contains(action_name)) {
            continue;
        }
        m_oper_in_group.emplace(action_name, action_name);
    }

    for (const auto& [group_name, oper_name] : m_oper_in_group) {
        auto& this_group = get_combat_data().groups[group_name];
        // there is a build error on macOS
        // https://github.com/MaaAssistantArknights/MaaAssistantArknights/actions/runs/3779762713/jobs/6425284487
        const std::string& oper_name_for_lambda = oper_name;
        auto iter = ranges::find_if(this_group, [&](const auto& oper) { return oper.name == oper_name_for_lambda; });
        if (iter == this_group.end()) {
            continue;
        }
        m_skill_usage.emplace(oper_name, iter->skill_usage);
        m_skill_times.emplace(oper_name, iter->skill_times);
    }

    return true;
}

bool asst::BattleProcessTask::do_action(const battle::copilot::Action& action, size_t index)
{
    LogTraceFunction;

    notify_action(action);

    if (!wait_condition(action)) {
        return false;
    }
    if (action.pre_delay > 0) {
        sleep_and_do_strategy(action.pre_delay);
        // 等待之后画面可能会变化，更新下干员信息
        update_deployment();
    }

    bool ret = false;
    const std::string& name = get_name_from_group(action.name);
    const auto& location = action.location;

    switch (action.type) {
    case ActionType::Deploy:
        ret = deploy_oper(name, location, action.direction);
        if (ret) m_in_bullet_time = false;
        break;

    case ActionType::Retreat:
        ret = m_in_bullet_time ? click_retreat() : (location.empty() ? retreat_oper(name) : retreat_oper(location));
        if (ret) m_in_bullet_time = false;
        break;

    case ActionType::UseSkill:
        ret = m_in_bullet_time ? click_skill() : (location.empty() ? use_skill(name) : use_skill(location));
        if (ret) m_in_bullet_time = false;
        break;

    case ActionType::SwitchSpeed:
        ret = speed_up();
        break;

    case ActionType::BulletTime:
        ret = enter_bullet_time(name, location);
        if (ret) m_in_bullet_time = true;
        break;

    case ActionType::SkillUsage:
        m_skill_usage[action.name] = action.modify_usage;
        if (action.modify_usage == SkillUsage::Times) 
            m_skill_times[action.name] = action.modify_times;
        ret = true;
        break;

    case ActionType::Output:
        // DoNothing
        ret = true;
        break;

    case ActionType::MoveCamera:
        ret = move_camera(action.distance);
        break;

    case ActionType::SkillDaemon:
        ret = wait_until_end();
        break;
    default:
        ret = do_derived_action(action, index);
        break;
    }

    sleep_and_do_strategy(action.post_delay);

    return ret;
}

const std::string& asst::BattleProcessTask::get_name_from_group(const std::string& action_name)
{
    auto iter = m_oper_in_group.find(action_name);
    if (iter != m_oper_in_group.cend()) {
        return iter->second;
    }
    m_oper_in_group.emplace(action_name, action_name);
    return m_oper_in_group.at(action_name);
}

void asst::BattleProcessTask::notify_action(const battle::copilot::Action& action)
{
    const static std::unordered_map<ActionType, std::string> ActionNames = {
        { ActionType::Deploy, "Deploy" },
        { ActionType::UseSkill, "UseSkill" },
        { ActionType::Retreat, "Retreat" },
        { ActionType::SkillDaemon, "SkillDaemon" },
        { ActionType::SwitchSpeed, "SwitchSpeed" },
        { ActionType::SkillUsage, "SkillUsage" },
        { ActionType::BulletTime, "BulletTime" },
        { ActionType::Output, "Output" },
        { ActionType::MoveCamera, "MoveCamera" },
        { ActionType::DrawCard, "DrawCard" },
        { ActionType::CheckIfStartOver, "CheckIfStartOver" },
    };

    json::value info = basic_info_with_what("CopilotAction");
    info["details"] |= json::object {
        { "action", ActionNames.at(action.type) },
        { "target", action.name },
        { "doc", action.doc },
        { "doc_color", action.doc_color },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::BattleProcessTask::wait_condition(const Action& action)
{
    cv::Mat image;
    auto update_image_if_empty = [&]() {
        if (image.empty()) {
            image = ctrler()->get_image();
            check_in_battle(image);
        }
    };
    auto do_strategy_and_update_image = [&]() {
        do_strategic_action(image);
        image = ctrler()->get_image();
    };

    if (action.cost_changes != 0) {
        update_image_if_empty();
        update_cost(image);
        int pre_cost = m_cost;

        while (!need_exit()) {
            update_cost(image);
            if (action.cost_changes != 0) {
                if ((pre_cost + action.cost_changes < 0) ? (m_cost <= pre_cost + action.cost_changes)
                                                         : (m_cost >= pre_cost + action.cost_changes)) {
                    break;
                }
            }
            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image();
        }
    }

    if (m_kills < action.kills) {
        update_image_if_empty();
        while (!need_exit() && m_kills < action.kills) {
            update_kills(image);
            if (m_kills >= action.kills) {
                break;
            }
            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image();
        }
    }

    if (action.costs) {
        update_image_if_empty();
        while (!need_exit()) {
            update_cost(image);
            if (m_cost >= action.costs) {
                break;
            }
            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image();
        }
    }

    // 计算有几个干员在cd
    if (action.cooling >= 0) {
        update_image_if_empty();
        while (!need_exit()) {
            if (!update_deployment(false, image)) {
                return false;
            }
            size_t cooling_count = ranges::count_if(m_cur_deployment_opers | views::values,
                                                    [](const auto& oper) -> bool { return oper.cooling; });
            if (cooling_count == static_cast<size_t>(action.cooling)) {
                break;
            }
            do_strategy_and_update_image();
        }
    }

    // 部署干员还要额外等待费用够或 CD 转好
    if (action.type == ActionType::Deploy) {
        const std::string& name = get_name_from_group(action.name);
        update_image_if_empty();
        while (!need_exit()) {
            if (!update_deployment(false, image)) {
                return false;
            }
            if (auto iter = m_cur_deployment_opers.find(name);
                iter != m_cur_deployment_opers.cend() && iter->second.available) {
                break;
            }
            do_strategy_and_update_image();
        }
    }

    return true;
}

bool asst::BattleProcessTask::enter_bullet_time(const std::string& name, const Point& location)
{
    LogTraceFunction;

    bool ret = location.empty() ? click_oper_on_battlefield(name) : click_oper_on_battlefield(location);
    if (!ret) {
        ret = click_oper_on_deployment(name);
    };

    return ret;
}

void asst::BattleProcessTask::sleep_and_do_strategy(unsigned millisecond)
{
    LogTraceScope(__FUNCTION__ + std::string(" ms: ") + std::to_string(millisecond));

    using namespace std::chrono_literals;
    const auto start = std::chrono::steady_clock::now();
    const auto delay = millisecond * 1ms;

    while (!need_exit() && std::chrono::steady_clock::now() - start < delay) {
        do_strategic_action();
        std::this_thread::yield();
    }
}
