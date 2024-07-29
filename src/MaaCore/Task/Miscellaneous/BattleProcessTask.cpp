#include "BattleProcessTask.h"

#include "Utils/Ranges.hpp"
#include <chrono>
#include <future>
#include <thread>

#include "Utils/NoWarningCV.h"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Algorithm.hpp"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

using namespace asst::battle;
using namespace asst::battle::copilot;

asst::BattleProcessTask::BattleProcessTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain) :
    AbstractTask(callback, inst, task_chain),
    BattleHelper(inst)
{
}

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
        do_action_sync(action, i);
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
        details["level"] = stage_name;
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

void asst::BattleProcessTask::set_formation_task_ptr(
    std::shared_ptr<std::unordered_map<std::string, std::string>> value)
{
    m_formation_ptr = value;
}

bool asst::BattleProcessTask::to_group()
{
    std::unordered_map<std::string, std::vector<std::string>> groups;
    // 从编队任务中获取<干员-组名>映射
    if (m_formation_ptr != nullptr) {
        for (const auto& [group, oper] : *m_formation_ptr) {
            groups.emplace(oper, std::vector<std::string> { group });
        }
    }
    // 补充剩余的干员
    for (const auto& [group_name, oper_list] : get_combat_data().groups) {
        if (groups.contains(group_name)) {
            continue;
        }
        std::vector<std::string> oper_name_list;
        ranges::transform(oper_list, std::back_inserter(oper_name_list), [](const auto& oper) { return oper.name; });
        groups.emplace(group_name, std::move(oper_name_list));
    }

    std::unordered_set<std::string> char_set; // 干员集合
    for (const auto& oper : m_cur_deployment_opers) {
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
        if (!action.hasAvatarInfo()) {
            continue;
        }

        auto& avatar = action.getPayload<AvatarInfo>();
        const std::string& action_name = avatar.name;
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
    if (action.delay.pre_delay > 0) {
        sleep_and_do_strategy(action.delay.pre_delay);
        // 等待之后画面可能会变化，更新下干员信息
        update_deployment();
    }

    // 如果携带锚点编码，则存储快照
    save_snap_shot(action.point_code);

    bool ret = false;

    switch (action.type) {
    case ActionType::Deploy: {
        auto& avatar = action.getPayload<AvatarInfo>();

        const std::string& name = get_name_from_group(avatar.name);
        const auto& location = avatar.location;

        ret = deploy_oper(name, location, avatar.direction);
        if (ret) {
            m_in_bullet_time = false;
        }
    } break;
    case ActionType::Retreat: {
        auto& avatar = action.getPayload<AvatarInfo>();

        const std::string& name = get_name_from_group(avatar.name);
        const auto& location = avatar.location;

        ret = m_in_bullet_time ? click_retreat() : (location.empty() ? retreat_oper(name) : retreat_oper(location));
        if (ret) {
            m_in_bullet_time = false;
        }
    } break;
    case ActionType::UseSkill: {
        auto& avatar = action.getPayload<AvatarInfo>();

        const std::string& name = get_name_from_group(avatar.name);
        const auto& location = avatar.location;

        ret = m_in_bullet_time ? click_skill() : (location.empty() ? use_skill(name) : use_skill(location));
        if (ret) {
            m_in_bullet_time = false;
        }
    } break;
    case ActionType::SwitchSpeed:
        ret = speed_up();
        break;
    case ActionType::BulletTime: {
        auto& avatar = action.getPayload<AvatarInfo>();

        const std::string& name = get_name_from_group(avatar.name);
        const auto& location = avatar.location;

        ret = enter_bullet_time(name, location);
        if (ret) {
            m_in_bullet_time = true;
        }
    } break;
    case ActionType::SkillUsage: {
        auto& avatar = action.getPayload<AvatarInfo>();

        const std::string& name = get_name_from_group(avatar.name);

        m_skill_usage[name] = avatar.modify_usage;
        if (avatar.modify_usage == SkillUsage::Times) {
            m_skill_times[name] = avatar.modify_times;
        }
        ret = true;
    } break;
    case ActionType::Output:
        // DoNothing
        ret = true;
        break;
    case ActionType::MoveCamera: {
        auto& info = action.getPayload<MoveCameraInfo>();

        ret = move_camera(info.distance);
    } break;
    case ActionType::SkillDaemon:
        ret = wait_until_end();
        break;
    case ActionType::Loop: {
        auto& info = action.getPayload<LoopInfo>();

        // 假设被设置了自然数才赋值
        info.end_info.resetCounter();

        while (!check_condition(info.end_info)) {
            // 需要维护counter
            info.end_info.activeCounter();

            // 执行循环体
            for (size_t i = 0; i < info.loop_actions.size(); ++i) {
                if (need_exit() || !m_in_battle) {
                    goto END_LOOP;
                }

                if (info.continue_info.active() && check_condition(info.continue_info)) {
                    goto NEXT_LOOP;
                }

                if (info.break_info.active() && check_condition(info.break_info)) {
                    goto BREAK_LOOP;
                }

                ret &= do_action_sync(*info.loop_actions[i], i);
            }

NEXT_LOOP:
            continue;

BREAK_LOOP:
            break;
        }

END_LOOP:;
    } break;
    case ActionType::Case: {
        auto& info = action.getPayload<CaseInfo>();

        if (auto it = m_oper_in_group.find(info.group_select); it != m_oper_in_group.cend()) {
            // 没找到或者没在CaseInfo 中匹配就使用默认的
            // 能够找到干员就使用对应的case
            if (auto itFind = info.dispatch_actions.find(it->second); itFind != info.dispatch_actions.cend()) {
                for (size_t i = 0; i < itFind->second.size(); ++i) {
                    ret &= do_action_sync(*itFind->second[i], i);

                    if (need_exit() || !m_in_battle) {
                        ret = false;
                        break;
                    }
                }
            }
        }
        else {
            Log.warn("failed to find select group");
            ret = false;
        }
    } break;
    case ActionType::Until: {
        auto& info = action.getPayload<UntilInfo>();

        // 循环遍历携带的所有子动作，只要成功一个才退出
        // 防止死循环，添加loop_limit参数来限制循环
        action.trigger.resetCounter();
        switch (info.mode) {
        case TriggerInfo::Category::Any: {
            size_t idx = 0;
            while (ret == false) {
                // 到达循环极限，退出
                if (action.trigger.counter == action.trigger.count) {
                    ret = false;
                    break;
                }

                action.trigger.activeCounter();

                // 只要其中一个命令执行成功就退出
                size_t i = idx++ % info.candidate.size();
                if (do_action_async(*info.candidate[i], i)) {
                    ret = true;
                    break;
                }

                if (need_exit() || !m_in_battle) {
                    ret = false;
                    break;
                }
            }
        } break;
        case TriggerInfo::Category::All:
            [[fallthrough]];
        default: {
            std::set<ActionPtr> setSucc;
            size_t idx = 0;

            // 全部成功则完成循环
            while (setSucc.size() != info.candidate.size()) {
                // 到达循环极限，退出
                if (action.trigger.counter == action.trigger.count) {
                    ret = false;
                    break;
                }

                action.trigger.activeCounter();

                // 判断是否已经执行成功，如果已经成功就判断下一个
                size_t i = idx++ % info.candidate.size();
                if (setSucc.find(info.candidate[i]) != setSucc.end()) {
                    continue;
                }

                // 记录成功完成的动作
                if (do_action_async(*info.candidate[i], i)) {
                    setSucc.emplace(info.candidate[i]);
                }

                if (need_exit() || !m_in_battle) {
                    break;
                }
            }

        } break;
        }
        ret = true;

    } break;
    case ActionType::Check: {
        auto& info = action.getPayload<CheckInfo>();

        if (check_condition(info.condition_info)) {
            // 触发器满足条件
            size_t i = 0;
            for (; i < info.then_actions.size(); ++i) {
                if (!do_action_sync(*info.then_actions[i], i)) {
                    break;
                }

                if (need_exit() || !m_in_battle) {
                    break;
                }
            }

            ret = (i == info.then_actions.size());
        }
        else { // 触发器不满足条件
            size_t i = 0;
            for (; i < info.else_actions.size(); ++i) {
                if (!do_action_sync(*info.else_actions[i], i)) {
                    break;
                }

                if (need_exit() || !m_in_battle) {
                    break;
                }
            }

            ret = (i == info.else_actions.size());
        }

    } break;
    case ActionType::SavePoint:
        ret = true; // 锚点保存动作由于携带锚点编码，已经在前面进行存储
        break;
    case ActionType::SyncPoint: {
        auto& info = action.getPayload<PointInfo>();

        // 目标编码不匹配，退出
        if (!find_snap_shot(info.target_code)) {
            ret = false;
            break;
        }

        thread_local auto prev_frame_time = std::chrono::steady_clock::time_point {};
        static const auto min_frame_interval =
            std::chrono::milliseconds(Config.get_options().copilot_fight_screencap_interval);

        do {
            const auto now = std::chrono::steady_clock::now();

            if (check_point(info)) {
                break;
            }

            // prevent our program from consuming too much CPU
            if (prev_frame_time > now - min_frame_interval) [[unlikely]] {
                Log.debug("Sleeping for framerate limit");
                std::this_thread::sleep_for(min_frame_interval - (now - prev_frame_time));
            }
        } while (!need_exit() && m_in_battle);

        size_t i = 0;
        for (; i < info.then_actions.size(); ++i) {
            if (!do_action_sync(*info.then_actions[i], i)) {
                break;
            }

            if (need_exit() || !m_in_battle) {
                break;
            }
        }

        ret = (i == info.then_actions.size());
    } break;
    case ActionType::CheckPoint: {
        auto& info = action.getPayload<PointInfo>();

        // 目标编码不匹配，退出
        if (!find_snap_shot(info.target_code)) {
            ret = false;
            break;
        }

        if (check_point(info)) {
            size_t i = 0;
            for (; i < info.then_actions.size(); ++i) {
                if (!do_action_sync(*info.then_actions[i], i)) {
                    break;
                }

                if (need_exit() || !m_in_battle) {
                    break;
                }
            }

            ret = (i == info.then_actions.size());
        }
        else {
            size_t i = 0;
            for (; i < info.else_actions.size(); ++i) {
                if (!do_action_sync(*info.else_actions[i], i)) {
                    break;
                }

                if (need_exit() || !m_in_battle) {
                    break;
                }
            }

            ret = (i == info.then_actions.size());
        }
    } break;
    default:
        ret = do_derived_action(action, index);
        break;
    }

    sleep_and_do_strategy(action.delay.post_delay);

    return ret;
}

bool asst::BattleProcessTask::do_action_sync(const battle::copilot::Action& action, size_t index)
{
    LogTraceFunction;

    notify_action(action);

    thread_local auto prev_frame_time = std::chrono::steady_clock::time_point {};
    static const auto min_frame_interval =
        std::chrono::milliseconds(Config.get_options().copilot_fight_screencap_interval);

    // prevent our program from consuming too much CPU
    if (const auto now = std::chrono::steady_clock::now(); prev_frame_time > now - min_frame_interval) [[unlikely]] {
        Log.debug("Sleeping for framerate limit");
        std::this_thread::sleep_for(min_frame_interval - (now - prev_frame_time));
    }

    // 所有被设置的触发器都满足
    switch (action.trigger.category) {
    case TriggerInfo::Category::Succ: // 默认成功，跳过条件阶段，什么都不用做
        break;
    case TriggerInfo::Category::Any: {
        if (!wait_condition_any(action)) {
            return false;
        }
    } break;
    case TriggerInfo::Category::Not: {
        if (!wait_condition_not(action)) {
            return false;
        }
    } break;
    case TriggerInfo::Category::All:
        [[fallthrough]];
    default: {
        if (!wait_condition_all(action)) {
            return false;
        }
    } break;
    }

    // 部署干员还要额外等待费用够或 CD 转好
    if (action.type == ActionType::Deploy) {
        if (!wait_operator_ready(action)) {
            return false;
        }
    }

    prev_frame_time = std::chrono::steady_clock::now();

    return do_action(action, index);
}

bool asst::BattleProcessTask::do_action_async(const battle::copilot::Action& action, size_t index)
{
    LogTraceFunction;

    notify_action(action);

    thread_local auto prev_frame_time = std::chrono::steady_clock::time_point {};
    static const auto min_frame_interval =
        std::chrono::milliseconds(Config.get_options().copilot_fight_screencap_interval);

    // prevent our program from consuming too much CPU
    if (const auto now = std::chrono::steady_clock::now(); prev_frame_time > now - min_frame_interval) [[unlikely]] {
        Log.debug("Sleeping for framerate limit");
        std::this_thread::sleep_for(min_frame_interval - (now - prev_frame_time));
    }

    // 所有被设置的触发器都满足, 不等待
    if (!check_condition(action.trigger)) {
        return false;
    }

    // 部署干员还要额外等待费用够或 CD 转好
    if (action.type == ActionType::Deploy) {
        if (!wait_operator_ready(action)) {
            return false;
        }
    }

    prev_frame_time = std::chrono::steady_clock::now();

    return do_action(action, index);
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
        { ActionType::Loop, "Loop" },
        { ActionType::Case, "Case" },
        { ActionType::Check, "Check" },
        { ActionType::Until, "Until" },
        { ActionType::SavePoint, "SavePoint" },
        { ActionType::SyncPoint, "SyncPoint" },
        { ActionType::CheckPoint, "CheckPoint" },
    };

    json::value info = basic_info_with_what("CopilotAction");
    std::string strActionName;
    if (action.hasAvatarInfo()) {
        strActionName = action.getPayload<AvatarInfo>().name;
    }
    info["details"] |= json::object {
        { "action", ActionNames.at(action.type) },
        { "target", strActionName },
        { "doc", action.text.doc },
        { "doc_color", action.text.doc_color },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::BattleProcessTask::wait_operator_ready(const battle::copilot::Action& action)
{
    cv::Mat image;
    auto& avatarName = action.getPayload<AvatarInfo>().name;
    const std::string& name = get_name_from_group(avatarName);
    update_image_if_empty(&image);
    while (!need_exit()) {
        if (!update_deployment(false, image)) {
            return false;
        }
        if (auto iter = ranges::find_if(m_cur_deployment_opers, [&](const auto& oper) { return oper.name == name; });
            iter != m_cur_deployment_opers.end() && iter->available) {
            break;
        }
        do_strategy_and_update_image(&image);
    }

    return true;
}

void asst::BattleProcessTask::update_image_if_empty(cv::Mat* _Image)
{
    if (_Image->empty()) {
        (*_Image) = ctrler()->get_image();
        check_in_battle(*_Image);
    }
}

void asst::BattleProcessTask::do_strategy_and_update_image(cv::Mat* _Image)
{
    do_strategic_action(*_Image);
    (*_Image) = ctrler()->get_image();
}

// 等待至所有被设置的条件不被满足
bool asst::BattleProcessTask::wait_condition_not(const Action& action)
{
    cv::Mat image;

    // cost_changes 被指定才进入判断，且等待直至满足
    if (action.trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
        update_image_if_empty(&image);
        update_cost(image);
        int pre_cost = m_cost;

        while (!need_exit()) {
            update_cost(image);
            if (action.trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
                if ((pre_cost + action.trigger.cost_changes < 0) ? (m_cost <= pre_cost + action.trigger.cost_changes)
                                                                 : (m_cost >= pre_cost + action.trigger.cost_changes)) {
                    ;
                }
                else {
                    break;
                }

                if (!check_in_battle(image)) {
                    return false;
                }
                do_strategy_and_update_image(&image);
            }
        }
    }

    if (m_kills < action.trigger.kills) {
        update_image_if_empty(&image);
        while (!need_exit() && m_kills < action.trigger.kills) {
            update_kills(image);
            if (m_kills >= action.trigger.kills) {
                ;
            }
            else {
                break;
            }

            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image(&image);
        }
    }

    if (action.trigger.costs != TriggerInfo::DEACTIVE_COST) {
        update_image_if_empty(&image);
        while (!need_exit()) {
            update_cost(image);
            if (m_cost >= action.trigger.costs) {
                ;
            }
            else {
                break;
            }

            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image(&image);
        }
    }

    // 计算有几个干员在cd
    if (action.trigger.cooling > TriggerInfo::DEACTIVE_COOLING) {
        update_image_if_empty(&image);
        while (!need_exit()) {
            if (!update_deployment(false, image)) {
                return false;
            }
            size_t cooling_count =
                ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
            if (cooling_count == static_cast<size_t>(action.trigger.cooling)) {
                ;
            }
            else {
                break;
            }
            do_strategy_and_update_image(&image);
        }
    }

    return true;
}

// 等待至所有被设置的条件被满足
bool asst::BattleProcessTask::wait_condition_all(const Action& action)
{
    cv::Mat image;

    // cost_changes 被指定才进入判断，且等待直至满足
    if (action.trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
        update_image_if_empty(&image);
        update_cost(image);
        int pre_cost = m_cost;

        while (!need_exit()) {
            update_cost(image);
            if (action.trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
                if ((pre_cost + action.trigger.cost_changes < 0) ? (m_cost <= pre_cost + action.trigger.cost_changes)
                                                                 : (m_cost >= pre_cost + action.trigger.cost_changes)) {
                    break;
                }
            }
            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image(&image);
        }
    }

    if (m_kills < action.trigger.kills) {
        update_image_if_empty(&image);
        while (!need_exit() && m_kills < action.trigger.kills) {
            update_kills(image);
            if (m_kills >= action.trigger.kills) {
                break;
            }
            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image(&image);
        }
    }

    if (action.trigger.costs != TriggerInfo::DEACTIVE_COST) {
        update_image_if_empty(&image);
        while (!need_exit()) {
            update_cost(image);
            if (m_cost >= action.trigger.costs) {
                break;
            }
            if (!check_in_battle(image)) {
                return false;
            }
            do_strategy_and_update_image(&image);
        }
    }

    // 计算有几个干员在cd
    if (action.trigger.cooling > TriggerInfo::DEACTIVE_COOLING) {
        update_image_if_empty(&image);
        while (!need_exit()) {
            if (!update_deployment(false, image)) {
                return false;
            }
            size_t cooling_count =
                ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
            if (cooling_count == static_cast<size_t>(action.trigger.cooling)) {
                break;
            }
            do_strategy_and_update_image(&image);
        }
    }

    return true;
}

// 等待至被设定的任意一个条件被满足
bool asst::BattleProcessTask::wait_condition_any(const Action& action)
{
    cv::Mat image;

    // 提前准备好快照，便于后续设置判断费用差距
    if (action.trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
        update_image_if_empty(&image);
        update_cost(image);
    }
    int pre_cost = m_cost;

    while (!need_exit()) {
        update_image_if_empty(&image);

        if (action.trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
            update_cost(image);
            if ((pre_cost + action.trigger.cost_changes < 0) ? (m_cost <= pre_cost + action.trigger.cost_changes)
                                                             : (m_cost >= pre_cost + action.trigger.cost_changes)) {
                return true;
            }
        }

        if (action.trigger.kills != TriggerInfo::DEACTIVE_KILLS) {
            update_kills(image);
            if (m_kills >= action.trigger.kills) {
                return true;
            }
        }

        if (action.trigger.costs != TriggerInfo::DEACTIVE_COST) {
            update_cost(image);
            if (m_cost >= action.trigger.costs) {
                return true;
            }
        }

        // 计算有几个干员在cd
        if (action.trigger.cooling != TriggerInfo::DEACTIVE_COOLING) {
            if (update_deployment(false, image)) {
                size_t cooling_count =
                    ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
                if (cooling_count == static_cast<size_t>(action.trigger.cooling)) {
                    return true;
                }
            }
        }

        if (!check_in_battle(image)) {
            return false;
        }

        do_strategy_and_update_image(&image);
    }

    return false;
}

bool asst::BattleProcessTask::check_condition_not(const battle::copilot::TriggerInfo& _Trigger)
{
    using TriggerInfo = battle::copilot::TriggerInfo;

    cv::Mat image;

    update_image_if_empty(&image);

    if (_Trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
        int pre_cost = m_cost;
        update_cost(image);

        if (_Trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
            if ((pre_cost + _Trigger.cost_changes < 0) ? (m_cost <= pre_cost + _Trigger.cost_changes)
                                                       : (m_cost >= pre_cost + _Trigger.cost_changes)) {
                return false;
            }
        }
    }

    if (_Trigger.kills != TriggerInfo::DEACTIVE_KILLS) {
        update_kills(image);
        if (m_kills >= _Trigger.kills) {
            return false;
        }
    }

    if (_Trigger.costs != TriggerInfo::DEACTIVE_COST) {
        update_cost(image);
        if (m_cost >= _Trigger.costs) {
            return false;
        }
    }

    // 计算有几个干员在cd
    if (_Trigger.cooling != TriggerInfo::DEACTIVE_COOLING) {
        if (update_deployment(false, image)) {
            size_t cooling_count =
                ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
            if (cooling_count == static_cast<size_t>(_Trigger.cooling)) {
                return false;
            }
        }
    }

    if (_Trigger.count != TriggerInfo::DEACTIVE_COUNT) {
        if (_Trigger.counter == _Trigger.count) {
            return false;
        }
    }

    do_strategy_and_update_image(&image);

    return true;
}

bool asst::BattleProcessTask::check_condition_all(const battle::copilot::TriggerInfo& _Trigger)
{
    using TriggerInfo = battle::copilot::TriggerInfo;

    cv::Mat image;

    update_image_if_empty(&image);

    if (_Trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
        int pre_cost = m_cost;
        update_cost(image);

        if (_Trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
            if ((pre_cost + _Trigger.cost_changes < 0) ? (m_cost <= pre_cost + _Trigger.cost_changes)
                                                       : (m_cost >= pre_cost + _Trigger.cost_changes)) {
                ;
            }
            else {
                return false;
            }
        }
    }

    if (_Trigger.kills != TriggerInfo::DEACTIVE_KILLS) {
        update_kills(image);
        if (m_kills >= _Trigger.kills) {
            ;
        }
        else {
            return false;
        }
    }

    if (_Trigger.costs != TriggerInfo::DEACTIVE_COST) {
        update_cost(image);
        if (m_cost >= _Trigger.costs) {
            ;
        }
        else {
            return false;
        }
    }

    // 计算有几个干员在cd
    if (_Trigger.cooling != TriggerInfo::DEACTIVE_COOLING) {
        if (!update_deployment(false, image)) {
            return false;
        }
        size_t cooling_count =
            ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
        if (cooling_count == static_cast<size_t>(_Trigger.cooling)) {
            ;
        }
        else {
            return false;
        }
    }

    if (_Trigger.count != TriggerInfo::DEACTIVE_COUNT) {
        if (_Trigger.counter == _Trigger.count) {
            ;
        }
        else {
            return false;
        }
    }

    do_strategy_and_update_image(&image);

    return true;
}

bool asst::BattleProcessTask::check_condition_any(const battle::copilot::TriggerInfo& _Trigger)
{
    using TriggerInfo = battle::copilot::TriggerInfo;

    cv::Mat image;

    update_image_if_empty(&image);
    if (_Trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
        int pre_cost = m_cost;
        update_cost(image);

        if (_Trigger.cost_changes != TriggerInfo::DEACTIVE_COST_CHANGES) {
            if ((pre_cost + _Trigger.cost_changes < 0) ? (m_cost <= pre_cost + _Trigger.cost_changes)
                                                       : (m_cost >= pre_cost + _Trigger.cost_changes)) {
                return true;
            }
        }
    }

    if (_Trigger.kills != TriggerInfo::DEACTIVE_KILLS) {
        update_kills(image);
        if (m_kills >= _Trigger.kills) {
            return true;
        }
    }

    if (_Trigger.costs != TriggerInfo::DEACTIVE_COST) {
        update_cost(image);
        if (m_cost >= _Trigger.costs) {
            return true;
        }
    }

    // 计算有几个干员在cd
    if (_Trigger.cooling > TriggerInfo::DEACTIVE_COOLING) {
        if (update_deployment(false, image)) {
            size_t cooling_count =
                ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
            if (cooling_count == static_cast<size_t>(_Trigger.cooling)) {
                return true;
            }
        }
    }

    if (_Trigger.count != TriggerInfo::DEACTIVE_COUNT) {
        if (_Trigger.counter == _Trigger.count) {
            return true;
        }
    }

    do_strategy_and_update_image(&image);

    return false;
}

bool asst::BattleProcessTask::check_condition(const battle::copilot::TriggerInfo& _Trigger)
{
    switch (_Trigger.category) {
    case TriggerInfo::Category::Succ: // 默认成功，跳过条件阶段，什么都不用做
        break;
    case TriggerInfo::Category::Any: {
        if (!check_condition_any(_Trigger)) {
            return false;
        }
    } break;
    case TriggerInfo::Category::Not: {
        if (!check_condition_not(_Trigger)) {
            return false;
        }
    } break;
    case TriggerInfo::Category::All:
        [[fallthrough]];
    default: {
        if (!check_condition_all(_Trigger)) {
            return false;
        }
    } break;
    }

    return true;
}

bool asst::BattleProcessTask::check_point(const battle::copilot::PointInfo& _Current)
{
    switch (_Current.mode) {
    case TriggerInfo::Category::Succ: // 默认成功，跳过条件阶段，什么都不用做
        break;
    case TriggerInfo::Category::Any: {
        if (!check_point_any(_Current)) {
            return false;
        }
    } break;
    case TriggerInfo::Category::Not: {
        if (!check_point_not(_Current)) {
            return false;
        }
    } break;
    case TriggerInfo::Category::All:
        [[fallthrough]];
    default: {
        if (!check_point_all(_Current)) {
            return false;
        }
    } break;
    }

    return true;
}

bool asst::BattleProcessTask::check_point_not(const battle::copilot::PointInfo& _Current)
{
    using TriggerInfo = battle::copilot::TriggerInfo;

    auto& target = get_snap_shot(_Current.target_code);
    auto now = gen_snap_shot();

    if (_Current.range.first.kills != TriggerInfo::DEACTIVE_KILLS) {
        auto diff = now.kills - target.kills;
        if ((_Current.range.first.kills <= diff) && (diff <= _Current.range.second.kills)) {
            return false;
        }
    }

    if (_Current.range.first.cost != TriggerInfo::DEACTIVE_COST) {
        auto diff = now.cost - target.cost;
        if ((_Current.range.first.cost <= diff) && (diff <= _Current.range.second.cost)) {
            return false;
        }
    }

    if (_Current.range.first.cooling_count != TriggerInfo::DEACTIVE_COST) {
        auto diff = now.cooling_count - target.cooling_count;
        if ((_Current.range.first.cooling_count <= diff) && (diff <= _Current.range.second.cooling_count)) {
            return false;
        }
    }

    if (_Current.range.first.interval != 0 || _Current.range.second.interval != 0) {
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now.tNow - target.tNow).count();
        if ((_Current.range.first.interval <= diff) && (diff <= _Current.range.second.interval)) {
            return false;
        }
    }

    return true;
}

bool asst::BattleProcessTask::check_point_all(const battle::copilot::PointInfo& _Current)
{
    using TriggerInfo = battle::copilot::TriggerInfo;

    auto& target = get_snap_shot(_Current.target_code);
    auto now = gen_snap_shot();

    if (_Current.range.first.kills != TriggerInfo::DEACTIVE_KILLS) {
        auto diff = now.kills - target.kills;
        if ((_Current.range.first.kills <= diff) && (diff <= _Current.range.second.kills)) {
            ;
        }
        else {
            return false;
        }
    }

    if (_Current.range.first.cost != TriggerInfo::DEACTIVE_COST) {
        auto diff = now.cost - target.cost;
        if ((_Current.range.first.cost <= diff) && (diff <= _Current.range.second.cost)) {
            ;
        }
        else {
            return false;
        }
    }

    if (_Current.range.first.cooling_count != TriggerInfo::DEACTIVE_COST) {
        auto diff = now.cooling_count - target.cooling_count;
        if ((_Current.range.first.cooling_count <= diff) && (diff <= _Current.range.second.cooling_count)) {
            ;
        }
        else {
            return false;
        }
    }

    if (_Current.range.first.interval != 0 || _Current.range.second.interval != 0) {
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now.tNow - target.tNow).count();
        if ((_Current.range.first.interval <= diff) && (diff <= _Current.range.second.interval)) {
            ;
        }
        else {
            return false;
        }
    }

    return true;
}

bool asst::BattleProcessTask::check_point_any(const battle::copilot::PointInfo& _Current)
{
    using TriggerInfo = battle::copilot::TriggerInfo;

    auto& target = get_snap_shot(_Current.target_code);
    auto now = gen_snap_shot();

    if (_Current.range.first.kills != TriggerInfo::DEACTIVE_KILLS) {
        auto diff = now.kills - target.kills;
        if ((_Current.range.first.kills <= diff) && (diff <= _Current.range.second.kills)) {
            return true;
        }
    }

    if (_Current.range.first.cost != TriggerInfo::DEACTIVE_COST) {
        auto diff = now.cost - target.cost;
        if ((_Current.range.first.cost <= diff) && (diff <= _Current.range.second.cost)) {
            return true;
        }
    }

    if (_Current.range.first.cooling_count != TriggerInfo::DEACTIVE_COST) {
        auto diff = now.cooling_count - target.cooling_count;
        if ((_Current.range.first.cooling_count <= diff) && (diff <= _Current.range.second.cooling_count)) {
            return true;
        }
    }

    if (_Current.range.first.interval != 0 || _Current.range.second.interval != 0) {
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now.tNow - target.tNow).count();
        if ((_Current.range.first.interval <= diff) && (diff <= _Current.range.second.interval)) {
            return true;
        }
    }

    return false;
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

auto asst::BattleProcessTask::gen_snap_shot() -> battle::copilot::PointInfo::SnapShot
{
    cv::Mat image;

    update_image_if_empty(&image);

    update_cost(image);

    battle::copilot::PointInfo::SnapShot shot;

    shot.cost = m_cost;

    update_kills(image);
    shot.kills = m_kills;

    if (update_deployment(false, image)) {
        shot.cooling_count =
            ranges::count_if(m_cur_deployment_opers, [](const auto& oper) -> bool { return oper.cooling; });
    }

    shot.tNow = std::chrono::steady_clock::now();

    do_strategy_and_update_image(&image);

    return shot;
}

bool asst::BattleProcessTask::find_snap_shot(const std::string& code) const noexcept
{
    return m_snap_shots.find(code) != m_snap_shots.cend();
}

void asst::BattleProcessTask::save_snap_shot(const std::string& code)
{
    if (code.empty()) {
        return;
    }

    m_snap_shots[code] = gen_snap_shot();
}

auto asst::BattleProcessTask::get_snap_shot(const std::string& code) const noexcept
    -> battle::copilot::PointInfo::SnapShot const&
{
    return m_snap_shots.at(code);
}

bool asst::BattleProcessTask::check_in_battle(const cv::Mat& reusable, bool weak)
{
    LogTraceFunction;

    cv::Mat image = reusable.empty() ? ctrler()->get_image() : reusable;

    if (weak) {
        BattlefieldMatcher analyzer(image);
        auto result = analyzer.analyze();
        m_in_battle = result.has_value();
        if (m_in_battle && !result->pause_button) {
            if (check_skip_plot_button(image) && check_in_speed_up(image)) {
                speed_up();
            }
        }
    }
    else {
        check_skip_plot_button(image);
        m_in_battle = check_pause_button(image);
    }
    return m_in_battle;
}
