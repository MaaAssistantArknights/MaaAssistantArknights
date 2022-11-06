#include "BattleProcessTask.h"

#include "Utils/AsstRanges.hpp"
#include <chrono>
#include <future>
#include <thread>

#include "Utils/AsstRanges.hpp"
#include "Utils/NoWarningCV.h"

#include "Controller.h"
#include "ImageAnalyzer/BattleImageAnalyzer.h"
#include "ImageAnalyzer/General/MatchImageAnalyzer.h"
#include "ImageAnalyzer/General/OcrWithPreprocessImageAnalyzer.h"
#include "ProcessTask.h"
#include "Resource/CopilotConfiger.h"
#include "Resource/TilePack.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

#include "Utils/AsstImageIo.hpp"

void asst::BattleProcessTask::set_stage_name(std::string name)
{
    m_stage_name = std::move(name);
}

bool asst::BattleProcessTask::_run()
{
    bool ret = get_stage_info();

    if (!ret) {
        json::value info = basic_info_with_what("UnsupportedLevel");
        auto& details = info["details"];
        details["level"] = m_stage_name;
        callback(AsstMsg::SubTaskExtraInfo, info);
        return false;
    }

    {
        json::value info = basic_info_with_what("BattleActionDoc");
        auto& details = info["details"];
        details["title"] = m_copilot_data.title;
        details["title_color"] = m_copilot_data.title_color;
        details["details"] = m_copilot_data.details;
        details["details_color"] = m_copilot_data.details_color;
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    while (!need_exit() && !analyze_opers_preview()) {
        std::this_thread::yield();
    }

    for (const auto& action : m_copilot_data.actions) {
        if (need_exit()) {
            break;
        }
        do_action(action);
    }

    return true;
}

bool asst::BattleProcessTask::get_stage_info()
{
    LogTraceFunction;

    m_normal_tile_info = Tile.calc(m_stage_name, false);
    m_side_tile_info = Tile.calc(m_stage_name, true);

    if (m_side_tile_info.empty() || m_normal_tile_info.empty()) {
        return false;
    }

    const auto& copilot = Copilot;
    bool contains = copilot.contains_actions(m_stage_name);
    if (!contains) {
        return false;
    }

    m_copilot_data = copilot.get_actions(m_stage_name);

    return true;
}

bool asst::BattleProcessTask::analyze_opers_preview()
{
    {
        json::value info = basic_info_with_what("BattleAction");
        auto& details = info["details"];
        details["action"] = "识别干员";
        details["doc"] = "";
        details["doc_color"] = "";
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    MatchImageAnalyzer officially_begin_analyzer;
    officially_begin_analyzer.set_task_info("BattleOfficiallyBegin");
    cv::Mat image;
    while (!need_exit()) {
        image = m_ctrler->get_image();
        officially_begin_analyzer.set_image(image);
        if (officially_begin_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    BattleImageAnalyzer oper_analyzer;
    oper_analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    while (!need_exit()) {
        image = m_ctrler->get_image();
        oper_analyzer.set_image(image);
        if (oper_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    // 识别一帧总击杀数
    BattleImageAnalyzer kills_analyzer(image);
    kills_analyzer.set_target(BattleImageAnalyzer::Target::Kills);
    if (kills_analyzer.analyze()) {
        m_kills = kills_analyzer.get_kills();
        m_total_kills = kills_analyzer.get_total_kills();
    }
    auto draw = image.clone();

    // 暂停游戏准备识别干员
    // 在刚进入游戏的时候（画面刚刚完全亮起来的时候），点暂停是没反应的
    // 所以这里一直点，直到真的点上了为止
    while (!need_exit()) {
        battle_pause();
        image = m_ctrler->get_image();
        officially_begin_analyzer.set_image(image);
        if (!officially_begin_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    auto draw_future = std::async(std::launch::async, [&]() {
        std::filesystem::create_directory("map");
        for (const auto& [loc, info] : m_normal_tile_info) {
            std::string text = "( " + std::to_string(loc.x) + ", " + std::to_string(loc.y) + " )";
            cv::putText(draw, text, cv::Point(info.pos.x - 30, info.pos.y), 1, 1.2, cv::Scalar(0, 0, 255), 2);
        }
        asst::imwrite("map/" + m_stage_name + ".png", draw);
    });

    auto opers = oper_analyzer.get_opers();

    Rect cur_rect;
    int click_delay = Task.get("BattleUseOper")->pre_delay;
    for (size_t i = 0; i != opers.size(); ++i) {
        Log.trace(__FUNCTION__, "ready to click No.", i, "oper");
        const auto& cur_oper = oper_analyzer.get_opers();
        size_t offset = opers.size() > cur_oper.size() ? opers.size() - cur_oper.size() : 0;
        cur_rect = cur_oper.at(i - offset).rect;
        m_ctrler->click(cur_rect);
        sleep(click_delay);

        image = m_ctrler->get_image();

        OcrWithPreprocessImageAnalyzer name_analyzer(image);
        name_analyzer.set_task_info("BattleOperName");
        name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

        std::string oper_name = "Unknown";
        if (name_analyzer.analyze()) {
            name_analyzer.sort_result_by_score();
            oper_name = name_analyzer.get_result().front().text;
        }
        opers.at(i).name = oper_name;
        Log.info(__FUNCTION__, "oper's name", oper_name);

        bool not_found = true;
        // 找出这个干员是哪个组里的，以及他的技能用法等
        for (const auto& [group_name, deploy_opers] : m_copilot_data.groups) {
            auto iter = ranges::find_if(
                deploy_opers, [&](const BattleDeployOper& deploy) -> bool { return deploy.name == oper_name; });
            if (iter != deploy_opers.cend()) {
                m_group_to_oper_mapping.emplace(group_name, *iter);
                not_found = false;
                break;
            }
        }
        // 没找到，可能是召唤物等新出现的
        if (not_found) {
            m_group_to_oper_mapping.emplace(oper_name, BattleDeployOper { oper_name });
        }

        m_cur_opers_info.emplace(std::move(oper_name), std::move(opers.at(i)));

        // 干员特别多的时候，任意干员被点开，都会导致下方的干员图标被裁剪和移动。所以这里需要重新识别一下
        Log.trace(__FUNCTION__, "ready to analyze oper again");
        oper_analyzer.set_image(image);
        oper_analyzer.analyze();
    }

    draw_future.wait();

    m_ctrler->click(cur_rect);
    sleep(click_delay);
    battle_pause();

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
    // 除非主动使用，不然可用干员数任何情况下都不会减少
    // 主动使用会 erase, 所以少了就是识别错了
    if (cur_opers_info.size() < m_cur_opers_info.size()) {
        Log.error(__FUNCTION__, "Decrease in staff, Just return");
        return false;
    }

    decltype(m_cur_opers_info) pre_opers_info;
    m_cur_opers_info.swap(pre_opers_info);

    const int size_change = static_cast<int>(cur_opers_info.size()) - static_cast<int>(pre_opers_info.size());
    for (const auto& cur_oper : cur_opers_info) {
        if (cur_oper.cooling) {
            continue;
        }
        // 该干员在上一帧中可能的位置。需要考虑召唤者退场&可用召唤物消失的情况，所以 lhs-1 rhs+1
        int left_index = std::max(0, static_cast<int>(cur_oper.index) - size_change - 1);
        int right_index = static_cast<int>(cur_oper.index + 1);

        std::vector<decltype(pre_opers_info)::const_iterator> ranged_iters;
        // 找出该干员可能对应的之前的谁
        for (auto iter = pre_opers_info.cbegin(); iter != pre_opers_info.cend(); ++iter) {
            int pre_index = static_cast<int>(iter->second.index);
            if (left_index <= pre_index && pre_index <= right_index) {
                ranged_iters.emplace_back(iter);
            }
        }

        // 干员也可能是撤退下来的，把所有已使用的都拿出来比较下
        for (auto iter = m_all_opers_info.cbegin(); iter != m_all_opers_info.cend(); ++iter) {
            const std::string& key = iter->first;
            if (!pre_opers_info.contains(key)) {
                ranged_iters.emplace_back(iter);
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

            OcrWithPreprocessImageAnalyzer name_analyzer(m_ctrler->get_image());
            name_analyzer.set_task_info("BattleOperName");
            name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

            if (name_analyzer.analyze()) {
                name_analyzer.sort_result_by_score();
                oper_name = name_analyzer.get_result().front().text;
            }
            m_group_to_oper_mapping[oper_name] = BattleDeployOper { oper_name };
            m_ctrler->click(cur_oper.rect);
            sleep(Task.get("BattleUseOper")->pre_delay);
            battle_pause();
        }
        else {
            oper_name = matched_iter->first;
            m_used_opers.erase(oper_name);
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
    json::value info = basic_info_with_what("BattleAction");
    auto& details = info["details"];
    std::string action_desc;
    switch (action.type) {
    case BattleActionType::Deploy:
        action_desc = "部署 " + action.group_name;
        break;
    case BattleActionType::Retreat:
        action_desc = "撤退 " + action.group_name;
        break;
    case BattleActionType::UseSkill:
        action_desc = "技能 " + action.group_name;
        break;
    case BattleActionType::SwitchSpeed:
        action_desc = "切换二倍速";
        break;
    case BattleActionType::SkillDaemon:
        action_desc = "摆完挂机";
        break;
    case BattleActionType::BulletTime:
        action_desc = "子弹时间";
        break;
        // TODO 其他情况
    case BattleActionType::SkillUsage:
    case BattleActionType::UseAllSkill:
    case BattleActionType::Output:;
    }
    details["action"] = action_desc;
    details["doc"] = action.doc;
    details["doc_color"] = action.doc_color;
    callback(AsstMsg::SubTaskExtraInfo, info);

    if (!wait_condition(action)) {
        return false;
    }
    if (action.pre_delay > 0) {
        sleep_with_possible_skill(action.pre_delay);
        // 等待之后画面可能会变化，再调用一次等待条件更新干员信息
        wait_condition(action);
    }

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
        // TODO
        break;
    case BattleActionType::SkillUsage: {
        auto& oper_info = m_group_to_oper_mapping[action.group_name];
        oper_info.skill_usage = action.modify_usage;
        m_used_opers[oper_info.name].info.skill_usage = action.modify_usage;
        ret = true;
    } break;
    case BattleActionType::Output:
        // DoNothing
        break;
    case BattleActionType::SkillDaemon:
        ret = wait_to_end(action);
        break;
        // TODO 其他情况
    case BattleActionType::UseAllSkill:;
    }
    sleep_with_possible_skill(action.post_delay);

    return ret;
}

bool asst::BattleProcessTask::wait_condition(const BattleAction& action)
{
    cv::Mat image = m_ctrler->get_image();

    // 计算初始状态
    int cost_base = -1;
    // int cooling_base = -1;

    if (action.cost_changes != 0) {
        BattleImageAnalyzer analyzer(image);
        analyzer.set_target(BattleImageAnalyzer::Target::Cost);
        if (analyzer.analyze()) {
            cost_base = analyzer.get_cost();
        }
    }
    // if (action.cooling != 0) {
    //     BattleImageAnalyzer analyzer(image);
    //     analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    //     if (analyzer.analyze()) {
    //         cooling_base =
    //             ranges::count_if(analyzer.get_opers(), [](const auto& oper) -> bool { return oper.cooling; });
    //     }
    // }

    // 计算击杀数
    while (m_kills < action.kills) {
        if (need_exit()) {
            return false;
        }
        BattleImageAnalyzer analyzer(image);
        if (m_total_kills) {
            analyzer.set_pre_total_kills(m_total_kills);
        }
        analyzer.set_target(BattleImageAnalyzer::Target::Kills);
        if (analyzer.analyze()) {
            m_kills = analyzer.get_kills();
            m_total_kills = analyzer.get_total_kills();
            if (m_kills >= action.kills) {
                break;
            }
        }

        try_possible_skill(image);
        std::this_thread::yield();
        image = m_ctrler->get_image();
    }

    // 计算费用变化量
    if (action.cost_changes) {
        while (true) {
            if (need_exit()) {
                return false;
            }
            BattleImageAnalyzer analyzer(image);
            analyzer.set_target(BattleImageAnalyzer::Target::Cost);
            if (analyzer.analyze()) {
                int cost = analyzer.get_cost();
                if (cost_base == -1) {
                    cost_base = cost;
                    image = m_ctrler->get_image();
                    continue;
                }
                if (cost >= cost_base + action.cost_changes) {
                    break;
                }
            }

            try_possible_skill(image);
            std::this_thread::yield();
            image = m_ctrler->get_image();
        }
    }

    // 计算有几个干员在cd
    if (action.cooling >= 0) {
        while (true) {
            if (need_exit()) {
                return false;
            }
            BattleImageAnalyzer analyzer(image);
            analyzer.set_target(BattleImageAnalyzer::Target::Oper);
            if (analyzer.analyze()) {
                int cooling_count = static_cast<int>(
                    ranges::count_if(analyzer.get_opers(), [](const auto& oper) -> bool { return oper.cooling; }));
                if (cooling_count == action.cooling) {
                    break;
                }
            }

            try_possible_skill(image);
            std::this_thread::yield();
            image = m_ctrler->get_image();
        }
    }

    // 部署干员还有额外等待费用够或 CD 转好
    if (action.type == BattleActionType::Deploy) {
        const std::string& name = m_group_to_oper_mapping[action.group_name].name;
        while (true) {
            if (need_exit()) {
                return false;
            }
            update_opers_info(image);

            if (auto iter = m_cur_opers_info.find(name); iter != m_cur_opers_info.cend() && iter->second.available) {
                break;
            }

            try_possible_skill(image);
            std::this_thread::yield();
            image = m_ctrler->get_image();
        }
    }

    return true;
}

bool asst::BattleProcessTask::oper_deploy(const BattleAction& action)
{
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    const auto& oper_info = m_group_to_oper_mapping[action.group_name];

    auto iter = m_cur_opers_info.find(oper_info.name);

    // 点击干员
    Rect oper_rect = iter->second.rect;
    m_ctrler->click(oper_rect);
    sleep(use_oper_task_ptr->pre_delay);

    // 拖动到场上
    Point placed_point = m_side_tile_info[action.location].pos;

    Rect placed_rect { placed_point.x, placed_point.y, 0, 0 };
    int dist = static_cast<int>(
        Point::distance(placed_point, { oper_rect.x + oper_rect.width / 2, oper_rect.y + oper_rect.height / 2 }));
    // 1000 是随便取的一个系数，把整数的 pre_delay 转成小数用的
    int duration = static_cast<int>(swipe_oper_task_ptr->pre_delay / 1000.0 * dist * log10(dist));
    m_ctrler->swipe(oper_rect, placed_rect, duration);

    sleep(use_oper_task_ptr->post_delay);

    // 拖动干员朝向
    if (action.direction != BattleDeployDirection::None) {
        static const std::unordered_map<BattleDeployDirection, Point> DirectionMapping = {
            { BattleDeployDirection::Right, Point(1, 0) }, { BattleDeployDirection::Down, Point(0, 1) },
            { BattleDeployDirection::Left, Point(-1, 0) }, { BattleDeployDirection::Up, Point(0, -1) },
            { BattleDeployDirection::None, Point(0, 0) },
        };

        // 计算往哪边拖动
        Point direction = DirectionMapping.at(action.direction);

        // 将方向转换为实际的 swipe end 坐标点
        constexpr int coeff = 500;
        Point end_point = placed_point + (direction * coeff);

        m_ctrler->swipe(placed_point, end_point, swipe_oper_task_ptr->post_delay);
        sleep(Task.get("BattleUseOper")->post_delay);
    }

    m_used_opers[iter->first] =
        BattleDeployInfo { action.location, m_normal_tile_info[action.location].pos, oper_info };

    m_cur_opers_info.erase(iter);

    // sleep(use_oper_task_ptr->pre_delay);

    return true;
}

bool asst::BattleProcessTask::oper_retreat(const BattleAction& action)
{
    const std::string& name = m_group_to_oper_mapping[action.group_name].name;
    Point pos;
    if (auto iter = m_used_opers.find(name);
        action.location.x == 0 && action.location.y == 0 && iter != m_used_opers.cend()) {
        pos = iter->second.pos;
        m_used_opers.erase(iter);
    }
    else {
        pos = m_normal_tile_info.at(action.location).pos;
    }
    m_ctrler->click(pos);
    sleep(Task.get("BattleUseOper")->pre_delay);

    return ProcessTask(*this, { "BattleOperRetreatJustClick" }).run();
}

bool asst::BattleProcessTask::use_skill(const BattleAction& action)
{
    const std::string& name = m_group_to_oper_mapping[action.group_name].name;
    Point pos;
    if (auto iter = m_used_opers.find(name);
        action.location.x == 0 && action.location.y == 0 && iter != m_used_opers.cend()) {
        pos = iter->second.pos;
    }
    else {
        pos = m_normal_tile_info.at(action.location).pos;
    }

    m_ctrler->click(pos);
    sleep(Task.get("BattleUseOper")->pre_delay);

    return ProcessTask(*this, { "BattleSkillReadyOnClick", "BattleSkillStopOnClick" })
        .set_task_delay(0)
        .set_retry_times(10000)
        .run();
}

bool asst::BattleProcessTask::wait_to_end(const BattleAction& action)
{
    std::ignore = action;

    MatchImageAnalyzer officially_begin_analyzer;
    officially_begin_analyzer.set_task_info("BattleOfficiallyBegin");
    cv::Mat image;
    while (!need_exit()) {
        image = m_ctrler->get_image();
        officially_begin_analyzer.set_image(image);
        if (!officially_begin_analyzer.analyze()) {
            break;
        }
        try_possible_skill(image);
        std::this_thread::yield();
    }
    return true;
}

bool asst::BattleProcessTask::try_possible_skill(const cv::Mat& image)
{
    auto task_ptr = Task.get("BattleAutoSkillFlag");
    const Rect& skill_roi_move = task_ptr->rect_move;

    MatchImageAnalyzer analyzer(image);
    analyzer.set_task_info(task_ptr);
    bool used = false;
    for (auto& info : m_used_opers | views::values) {
        if (info.info.skill_usage != BattleSkillUsage::Possibly && info.info.skill_usage != BattleSkillUsage::Once) {
            continue;
        }
        const Rect roi = Rect { info.pos.x, info.pos.y, 0, 0 }.move(skill_roi_move);
        analyzer.set_roi(roi);
        if (!analyzer.analyze()) {
            continue;
        }
        m_ctrler->click(info.pos);
        sleep(Task.get("BattleUseOper")->pre_delay);
        used |= ProcessTask(*this, { "BattleSkillReadyOnClick" }).set_task_delay(0).run();
        if (info.info.skill_usage == BattleSkillUsage::Once) {
            info.info.skill_usage = BattleSkillUsage::OnceUsed;
        }
    }
    return used;
}

void asst::BattleProcessTask::sleep_with_possible_skill(unsigned millisecond)
{
    if (need_exit()) {
        return;
    }
    if (millisecond == 0) {
        return;
    }
    auto start = std::chrono::steady_clock::now();
    long long duration = 0;

    Log.trace("ready to sleep_with_possible_skill", millisecond);

    while (!need_exit() && duration < millisecond) {
        duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        try_possible_skill(m_ctrler->get_image());
        std::this_thread::yield();
    }
    Log.trace("end of sleep_with_possible_skill", millisecond);
}

bool asst::BattleProcessTask::battle_pause()
{
    return ProcessTask(*this, { "BattlePause" }).run();
}

bool asst::BattleProcessTask::battle_speedup()
{
    return ProcessTask(*this, { "BattleSpeedUp" }).run();
}

template <typename GroupNameType, typename CharNameType>
std::optional<std::unordered_map<GroupNameType, CharNameType>> asst::BattleProcessTask::
    get_char_allocation_for_each_group(const std::unordered_map<GroupNameType, std::vector<CharNameType>>& group_list,
                                       const std::vector<CharNameType>& char_list)
{
    /*
     * * dlx 算法简介
     *
     * https://oi-wiki.org/search/dlx/
     *
     *
     * * dlx 算法作用
     *
     * 在形如:
     * a: 10010
     * b: 01110
     * c: 01001
     * d: 00100
     * e: 11010
     * 这样的数据里,
     * dlx 可以找到 {a, c, d} 这样每列恰好出现且仅出现一次 1 的数据,
     * 也即对全集的一个精确覆盖:
     * a: 10010
     * c: 01001
     * d: 00100
     *    11111
     *
     *
     * * dlx 算法建模
     *
     * dlx 的列分为 [组号] [干员号] 两部分
     * dlx 的行分为 [可能的选择对] [不选择该干员] 两部分
     *
     * [可能的选择对]:
     * 每行对应一种可能的选择,
     * 将组号，干员号对应位置的列设为1
     *
     * [不选择该干员]:
     * 每行对应不选择某干员的情况,
     * 将干员号对应位置的列设为1
     *
     *
     * * dlx 建模示例
     *
     * 有以下分组:
     * a: {1, 3, 4}
     * b: {2, 3, 5}
     * c: {1, 2, 3}
     * 拥有的干员:
     * {1, 2, 4, 5, 6}
     *
     * 先处理出所有可能的情况:
     * a: {1, 4}
     * b: {2, 5}
     * c: {1, 2}
     *
     * 构造表:
     *   abc 1245
     * 1 100 1000 <a, 1>
     * 2 100 0010 <a, 4>
     * 3 010 0100 <b, 2>
     * 4 010 0001 <b, 5>
     * 5 001 1000 <c, 1>
     * 6 001 0100 <c, 2>
     * 7 000 1000 ~1
     * 9 000 0100 ~2
     * 9 000 0010 ~4
     * A 000 0001 ~5
     *
     * 使用dlx求得一组解:
     * 一个可能的结果是:
     * 行号 {2, 3, 5, A}
     * 即 {<a, 4>, <b, 2>, <c, 1>, ~5}
     *
     * 输出分组结果:
     * a: 4
     * b: 2
     * c: 1
     *
     */

    // dlx 算法模板类
    class DancingLinksModel
    {
    private:
        size_t index {};
        std::vector<size_t> first, size;
        std::vector<size_t> left, right, up, down;
        std::vector<size_t> column, row;

        void remove(const size_t& column_id)
        {
            left[right[column_id]] = left[column_id];
            right[left[column_id]] = right[column_id];
            for (size_t i = down[column_id]; i != column_id; i = down[i]) {
                for (size_t j = right[i]; j != i; j = right[j]) {
                    up[down[j]] = up[j];
                    down[up[j]] = down[j];
                    --size[column[j]];
                }
            }
        }

        void recover(const size_t& column_id)
        {
            for (size_t i = up[column_id]; i != column_id; i = up[i]) {
                for (size_t j = left[i]; j != i; j = left[j]) {
                    up[down[j]] = down[up[j]] = j;
                    ++size[column[j]];
                }
            }
            left[right[column_id]] = right[left[column_id]] = column_id;
        }

    public:
        size_t answer_stack_size {};
        std::vector<size_t> answer_stack;

        DancingLinksModel(const size_t& max_node_num, const size_t& max_ans_size)
            : first(max_node_num), size(max_node_num), left(max_node_num), right(max_node_num), up(max_node_num),
              down(max_node_num), column(max_node_num), row(max_node_num), answer_stack(max_ans_size)
        {}

        void build(const size_t& column_id)
        {
            for (size_t i = 0; i <= column_id; i++) {
                left[i] = i - 1;
                right[i] = i + 1;
                up[i] = down[i] = i;
            }
            left[0] = column_id;
            right[column_id] = 0;
            index = column_id;
            first.clear();
            size.clear();
        }

        void insert(const size_t& row_id, const size_t& column_id)
        {
            column[++index] = column_id;
            row[index] = row_id;
            ++size[column_id];
            down[index] = down[column_id];
            up[down[column_id]] = index;
            up[index] = column_id;
            down[column_id] = index;
            if (!first[row_id]) {
                first[row_id] = left[index] = right[index] = index;
            }
            else {
                right[index] = right[first[row_id]];
                left[right[first[row_id]]] = index;
                left[index] = first[row_id];
                right[first[row_id]] = index;
            }
        }

        bool dance(const size_t& depth)
        {
            if (!right[0]) {
                answer_stack_size = depth;
                return true;
            }
            size_t column_id = right[0];
            for (size_t i = right[0]; i != 0; i = right[i]) {
                if (size[i] < size[column_id]) {
                    column_id = i;
                }
            }
            remove(column_id);
            for (size_t i = down[column_id]; i != column_id; i = down[i]) {
                answer_stack[depth] = row[i];
                for (size_t j = right[i]; j != i; j = right[j]) {
                    remove(column[j]);
                }
                if (dance(depth + 1)) {
                    return true;
                }
                for (size_t j = left[i]; j != i; j = left[j]) {
                    recover(column[j]);
                }
            }
            recover(column_id);
            return false;
        }
    };

    // 建立结点、组、干员与各自 id 的映射关系
    std::vector<std::pair<GroupNameType, CharNameType>> node_id_mapping;
    std::vector<GroupNameType> group_id_mapping;
    std::vector<CharNameType> char_id_mapping;
    std::unordered_map<GroupNameType, size_t> group_name_mapping;
    std::unordered_map<CharNameType, size_t> char_name_mapping;
    std::set<CharNameType> char_set(char_list.begin(), char_list.end());

    for (auto& i : group_list) {
        group_name_mapping[i.first] = group_id_mapping.size();
        group_id_mapping.emplace_back(i.first);
        for (auto& j : i.second) {
            if (char_set.contains(j)) {
                node_id_mapping.emplace_back(i.first, j);
                if (!char_name_mapping.contains(j)) {
                    char_name_mapping[j] = char_id_mapping.size();
                    char_id_mapping.emplace_back(j);
                }
            }
        }
    }

    // 建 01 矩阵
    const size_t node_num = node_id_mapping.size();
    const size_t group_num = group_id_mapping.size();
    const size_t char_num = char_id_mapping.size();

    DancingLinksModel dancing_links_model(2 * node_num + group_num + 2 * char_num + 1, group_num + char_num);

    dancing_links_model.build(group_num + char_num);

    for (size_t i = 0; i < node_num; i++) {
        dancing_links_model.insert(i + 1, group_name_mapping[node_id_mapping[i].first] + 1);
        dancing_links_model.insert(i + 1, group_num + char_name_mapping[node_id_mapping[i].second] + 1);
    }

    for (size_t i = 0; i < char_num; i++) {
        dancing_links_model.insert(i + node_num + 1, i + group_num + 1);
    }

    // dance!!
    bool has_solution = dancing_links_model.dance(0);

    // 判定结果
    if (!has_solution) return std::nullopt;

    std::unordered_map<GroupNameType, CharNameType> return_value;

    for (size_t i = 0; i < dancing_links_model.answer_stack_size; i++) {
        if (dancing_links_model.answer_stack[i] > node_num) break;
        return_value.insert(node_id_mapping[dancing_links_model.answer_stack[i] - 1]);
    }

    return return_value;
}
