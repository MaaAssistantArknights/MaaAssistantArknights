#include "BattleProcessTask.h"

#include "Controller.h"
#include "Resource.h"
#include "TaskData.h"
#include "Logger.hpp"

#include "ProcessTask.h"

#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "BattleImageAnalyzer.h"

void asst::BattleProcessTask::set_stage_name(std::string name)
{
    m_stage_name = std::move(name);
}

bool asst::BattleProcessTask::_run()
{
    bool ret = get_stage_info();

    if (!ret) {
        return false;
    }

    while (!analyze_opers_preview()) {
        ;
    }

    for (const auto& action : m_actions_group.actions) {
        do_action(action);
    }

    return true;
}

bool asst::BattleProcessTask::get_stage_info()
{
    const auto& tile = Resrc.tile();

    m_normal_tile_info = tile.calc(m_stage_name, false);
    m_side_tile_info = tile.calc(m_stage_name, true);

    //#ifdef ASST_DEBUG
    auto draw = m_ctrler->get_image();
    for (const auto& [loc, info] : m_normal_tile_info) {
        std::string text = "( " + std::to_string(loc.x) + ", " + std::to_string(loc.y) + " )";
        cv::putText(draw, text, cv::Point(info.pos.x - 30, info.pos.y), 1, 1.2, cv::Scalar(0, 0, 255), 2);
    }
    cv::imwrite(utils::utf8_to_gbk(m_stage_name) + ".png", draw);
    //#endif

    if (m_side_tile_info.empty() || m_normal_tile_info.empty()) {
        return false;
    }

    const auto& battle = Resrc.battle();
    bool contains = battle.contains_actions(m_stage_name);
    if (!contains) {
        return false;
    }

    m_actions_group = battle.get_actions(m_stage_name);

    return true;
}

bool asst::BattleProcessTask::analyze_opers_preview()
{
    BattleImageAnalyzer oper_analyzer(m_ctrler->get_image());
    oper_analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    if (!oper_analyzer.analyze()) {
        return false;
    }
    // TODO: 干员头像出来之后，还要过 2 秒左右才可以点击，这里可能还要加个延时

    battle_pause();

    auto opers = oper_analyzer.get_opers();

    for (size_t i = 0; i != opers.size(); ++i) {
        const auto& cur_oper = oper_analyzer.get_opers();
        size_t offset = opers.size() > cur_oper.size() ? opers.size() - cur_oper.size() : 0;
        m_ctrler->click(cur_oper.at(i - offset).rect);

        sleep(Task.get("BattleUseOper")->pre_delay);

        auto image = m_ctrler->get_image();

        OcrImageAnalyzer name_analyzer(image);
        name_analyzer.set_task_info("BattleOperName");
        name_analyzer.set_replace(
            std::dynamic_pointer_cast<OcrTaskInfo>(
                Task.get("Roguelike1RecruitData"))
            ->replace_map);

        std::string oper_name = "Unknown";
        if (name_analyzer.analyze()) {
            name_analyzer.sort_result_by_score();
            oper_name = name_analyzer.get_result().front().text;
        }
        opers.at(i).name = oper_name;

        bool not_found = true;
        // 找出这个干员是哪个组里的，以及他的技能用法等
        for (const auto& [group_name, deploy_opers] : m_actions_group.groups) {
            auto iter = std::find_if(deploy_opers.cbegin(), deploy_opers.cend(),
                [&](const BattleDeployOper& deploy) -> bool {
                    return deploy.name == oper_name;
                });
            if (iter != deploy_opers.cend()) {
                m_group_to_oper_mapping.emplace(group_name, *iter);
                not_found = false;
                break;
            }
        }
        // 没找到，可能是召唤物等新出现的
        if (not_found) {
            m_group_to_oper_mapping.emplace(oper_name, BattleDeployOper{ oper_name });
        }

        m_cur_opers_info.emplace(std::move(oper_name), std::move(opers.at(i)));

        // 干员特别多的时候，任意干员被点开，都会导致下方的干员图标被裁剪和移动。所以这里需要重新识别一下
        oper_analyzer.set_image(image);
        oper_analyzer.analyze();
    }
    battle_pause();
    cancel_selection();

    return true;
}

bool asst::BattleProcessTask::update_opers_info(const cv::Mat& image)
{
    BattleImageAnalyzer analyzer(image);
    analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& cur_opers_info = analyzer.get_opers();
    decltype(m_cur_opers_info) pre_opers_info;
    m_cur_opers_info.swap(pre_opers_info);

    const int size_change = static_cast<int>(cur_opers_info.size()) - static_cast<int>(pre_opers_info.size());
    const bool is_increased = size_change > 0;
    for (const auto& cur_oper : cur_opers_info) {
        int left_index = 0;
        int right_index = 0;
        // 干员数变多了，之前的干员可能位于 [当前位置 - |size_change|, 当前位置 ] 之间
        if (is_increased) {
            left_index = std::max(0, static_cast<int>(cur_oper.index) - size_change);
            right_index = static_cast<int>(cur_oper.index);
        }
        // 干员数减少了，之前的干员可能位于 [当前位置 , 当前位置 + |size_change|] 之间
        else {
            left_index = static_cast<int>(cur_oper.index);
            right_index = static_cast<int>(cur_oper.index) - size_change + 1; // size_change 是 负值
        }

        std::vector<decltype(pre_opers_info)::const_iterator> ranged_iters;
        // 找出该干员可能对应的之前的谁
        for (auto iter = pre_opers_info.cbegin(); iter != pre_opers_info.cend(); ++iter) {
            int pre_index = static_cast<int>(iter->second.index);
            if (left_index <= pre_index && pre_index <= right_index) {
                ranged_iters.emplace_back(iter);
            }
        }

        if (is_increased && !cur_oper.available) {
            continue;
        }
        // 为空说明上一帧画面里没有这个干员，可能是撤退下来的，把所有已使用的都拿出来比较下
        if (ranged_iters.empty()) {
            // 已使用的（可能是撤退下来的） = 所有干员 - 当前可用的干员
            // 求差集

            //std::set_difference(m_all_opers_info.cbegin(), m_all_opers_info.cend(),
            //    pre_opers_info.cbegin(), pre_opers_info.cend(),
            //    std::back_inserter(ranged_iters)
            //);
            // set_difference 返回的是元素，但我这里需要迭代器，所以只能手写了
            for (auto iter = m_all_opers_info.cbegin(); iter != m_all_opers_info.cend(); ++iter) {
                const std::string& key = iter->first;
                if (pre_opers_info.find(key) == pre_opers_info.cend()) {
                    ranged_iters.emplace_back(iter);
                }
            }
        }

        std::string oper_name = "Unknown";
        MatchRect matched_result;
        decltype(ranged_iters)::value_type matched_iter;

        MatchImageAnalyzer avatar_analyzer(cur_oper.avatar);
        avatar_analyzer.set_task_info("BattleAvatarData");
        // 遍历比较，得分最高的那个就说明是对应的那个
        for (const auto& iter : ranged_iters) {
            avatar_analyzer.set_templ(iter->second.avatar);
            if (!avatar_analyzer.analyze()) {
                continue;
            }
            if (matched_result.score < avatar_analyzer.get_result().score) {
                matched_result = avatar_analyzer.get_result();
                matched_iter = iter;
            }
        }
        // 一个都没匹配上，考虑是新增的召唤物，或者别的东西，点开来看一下
        if (matched_result.score == 0) {
            battle_pause();
            m_ctrler->click(cur_oper.rect);
            sleep(Task.get("BattleUseOper")->pre_delay);

            OcrImageAnalyzer name_analyzer(m_ctrler->get_image());
            name_analyzer.set_task_info("BattleOperName");
            name_analyzer.set_replace(
                std::dynamic_pointer_cast<OcrTaskInfo>(
                    Task.get("Roguelike1RecruitData"))
                ->replace_map);

            if (name_analyzer.analyze()) {
                name_analyzer.sort_result_by_score();
                oper_name = name_analyzer.get_result().front().text;
            }
            m_group_to_oper_mapping[oper_name] = BattleDeployOper{ oper_name };
            battle_pause();
            cancel_selection();
        }
        else {
            oper_name = matched_iter->first;
            if (is_increased) {
                m_used_opers.erase(oper_name);
            }
        }

        auto temp_oper = cur_oper;
        temp_oper.name = oper_name;
        // 保存当前干员信息
        m_all_opers_info[oper_name] = temp_oper;
        m_cur_opers_info.emplace(oper_name, std::move(temp_oper));
    }
    return true;
}

bool asst::BattleProcessTask::do_action(const BattleAction& action)
{
    if (!wait_condition(action)) {
        return false;
    }
    sleep(action.pre_delay);

    bool ret = false;
    switch (action.type) {
    case BattleActionType::Deploy:
        ret = oper_deploy(action);
        break;
    case BattleActionType::Retreat:
        ret = oper_retreat(action);
        break;
    case BattleActionType::UseSkill:
        ret = use_skill(action);
        break;
    case BattleActionType::SwitchSpeed:
        ret = battle_speedup();
        break;
    case BattleActionType::BulletTime:
        break;
    case BattleActionType::SkillUsage:
    {
        auto& oper_info = m_group_to_oper_mapping[action.group_name];
        oper_info.skill_usage = action.modify_usage;
        m_used_opers[oper_info.name].info.skill_usage = action.modify_usage;
        return true;
    } break;
    }
    sleep(action.rear_delay);

    return ret;
}

bool asst::BattleProcessTask::wait_condition(const BattleAction& action)
{
    while (m_kills < action.kills) {
        auto image = m_ctrler->get_image();
        BattleImageAnalyzer analyzer(image);

        analyzer.set_target(BattleImageAnalyzer::Target::Kills);
        if (analyzer.analyze()) {
            m_kills = analyzer.get_kills();
        }

        try_possible_skill(image);
    }

    return true;
}

bool asst::BattleProcessTask::oper_deploy(const BattleAction& action)
{
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    decltype(m_cur_opers_info)::iterator iter;
    BattleDeployOper oper_info;
    do {
        const auto& image = m_ctrler->get_image();

        update_opers_info(image);

        oper_info = m_group_to_oper_mapping[action.group_name];
        const std::string& name = oper_info.name;
        iter = m_cur_opers_info.find(name);
        if (iter == m_cur_opers_info.cend()) {
            Log.info("battle opers group", name, "not found");
        }

        try_possible_skill(image);
    } while (iter == m_cur_opers_info.cend() || !iter->second.available);

    // 点击干员
    Rect oper_rect = iter->second.rect;
    m_ctrler->click(oper_rect);
    sleep(use_oper_task_ptr->pre_delay);

    // 拖动到场上
    Point placed_point = m_side_tile_info[action.location].pos;

    Rect placed_rect{ placed_point.x ,placed_point.y, 1, 1 };
    m_ctrler->swipe(oper_rect, placed_rect, swipe_oper_task_ptr->pre_delay);

    sleep(use_oper_task_ptr->rear_delay);

    // 拖动干员朝向
    if (action.direction != BattleDeployDirection::None) {
        static const std::unordered_map<BattleDeployDirection, Point> DirectionMapping = {
            { BattleDeployDirection::Right, Point(1, 0)},
            { BattleDeployDirection::Down, Point(0, 1)},
            { BattleDeployDirection::Left, Point(-1, 0)},
            { BattleDeployDirection::Up, Point(0, -1)},
            { BattleDeployDirection::None, Point(0, 0)},
        };

        // 计算往哪边拖动
        Point direction = DirectionMapping.at(action.direction);

        // 将方向转换为实际的 swipe end 坐标点
        Point end_point = placed_point;
        constexpr int coeff = 500;
        end_point.x += direction.x * coeff;
        end_point.y += direction.y * coeff;

        end_point.x = std::max(0, end_point.x);
        end_point.x = std::min(end_point.x, WindowWidthDefault);
        end_point.y = std::max(0, end_point.y);
        end_point.y = std::min(end_point.y, WindowHeightDefault);

        m_ctrler->swipe(placed_point, end_point, swipe_oper_task_ptr->rear_delay);
    }

    m_used_opers[iter->first] = BattleDeployInfo{
        action.location,
        m_normal_tile_info[action.location].pos,
        std::move(oper_info) };

    m_cur_opers_info.erase(iter);

    //sleep(use_oper_task_ptr->pre_delay);

    return true;
}

bool asst::BattleProcessTask::oper_retreat(const BattleAction& action)
{
    const std::string& name = m_group_to_oper_mapping[action.group_name].name;
    auto iter = m_used_opers.find(name);
    if (iter == m_used_opers.cend()) {
        Log.error(name, " not used");
        return false;
    }
    Point pos = iter->second.pos;
    m_ctrler->click(pos);

    m_used_opers.erase(name);

    return ProcessTask(*this, { "BattleOperRetreat" }).run();
}

bool asst::BattleProcessTask::use_skill(const BattleAction& action)
{
    const std::string& name = m_group_to_oper_mapping[action.group_name].name;
    auto iter = m_used_opers.find(name);
    if (iter == m_used_opers.cend()) {
        Log.error(name, " not used");
        return false;
    }

    Point pos = iter->second.pos;
    m_ctrler->click(pos);

    return ProcessTask(*this, { "BattleSkillReadyOnClick" })
        .set_task_delay(0)
        .set_retry_times(10000)
        .run();
}

void asst::BattleProcessTask::try_possible_skill(const cv::Mat& image)
{
    static const Rect& skill_roi_move = Task.get("BattleAutoSkillFlag")->rect_move;

    MatchImageAnalyzer analyzer(image);
    analyzer.set_task_info("BattleAutoSkillFlag");
    for (auto& [name, info] : m_used_opers) {
        if (info.info.skill_usage != BattleSkillUsage::Possibly
            && info.info.skill_usage != BattleSkillUsage::Once) {
            continue;
        }
        const Rect roi = Rect{ info.pos.x, info.pos.y, 0, 0 }.move(skill_roi_move);
        analyzer.set_roi(roi);
        if (!analyzer.analyze()) {
            continue;
        }
        m_ctrler->click(info.pos);
        ProcessTask(*this, { "BattleSkillReadyOnClick" }).run();
        if (info.info.skill_usage == BattleSkillUsage::Once) {
            info.info.skill_usage = BattleSkillUsage::OnceUsed;
        }
    }
}

bool asst::BattleProcessTask::battle_pause()
{
    return ProcessTask(*this, { "BattlePause" }).run();
}

bool asst::BattleProcessTask::battle_speedup()
{
    return ProcessTask(*this, { "BattleSpeedUp" }).run();
}

bool asst::BattleProcessTask::cancel_selection()
{
    return ProcessTask(*this, { "BattleCancelSelection" }).run();
}
