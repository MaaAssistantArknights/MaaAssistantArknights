#include "RoguelikeBattleTaskPlugin.h"

#include "Utils/AsstRanges.hpp"
#include <chrono>
#include <future>

#include "Utils/NoWarningCV.h"

#include "../Sub/ProcessTask.h"
#include "Controller.h"
#include "ImageAnalyzer/BattleImageAnalyzer.h"
#include "ImageAnalyzer/General/MatchImageAnalyzer.h"
#include "ImageAnalyzer/General/OcrWithPreprocessImageAnalyzer.h"
#include "Resource/BattleDataConfiger.h"
#include "Resource/RoguelikeCopilotConfiger.h"
#include "Resource/TilePack.h"
#include "RuntimeStatus.h"
#include "TaskData.h"
#include "Utils/AsstImageIo.hpp"
#include "Utils/Logger.hpp"

bool asst::RoguelikeBattleTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1StartAction") {
        return true;
    }
    else {
        return false;
    }
}

void asst::RoguelikeBattleTaskPlugin::set_stage_name(std::string stage)
{
    m_stage_name = std::move(stage);
}

bool asst::RoguelikeBattleTaskPlugin::_run()
{
    using namespace std::chrono_literals;

    bool gotten_info = get_stage_info();
    if (!gotten_info) {
        return true;
    }
    if (!wait_start()) {
        return false;
    }

    speed_up();

    bool timeout = false;
    auto start_time = std::chrono::steady_clock::now();
    while (!need_exit()) {
        // 不在战斗场景，且已使用过了干员，说明已经打完了，就结束循环
        if (!auto_battle() && m_opers_used) {
            break;
        }
        if (std::chrono::steady_clock::now() - start_time > 5min) {
            timeout = true;
            break;
        }
    }
    // 超过时间限制了，一般是某个怪被干员卡住了，一直不结束。
    if (timeout) {
        Log.info("Timeout, retreat!");
        all_melee_retreat();

        start_time = std::chrono::steady_clock::now();
        while (!need_exit()) {
            if (!auto_battle()) {
                break;
            }
            if (std::chrono::steady_clock::now() - start_time > 1min) {
                Log.info("Timeout again, abandon!");
                abandon();
                break;
            }
        }
    }

    clear();

    return true;
}

void asst::RoguelikeBattleTaskPlugin::wait_for_start()
{
    ProcessTask(*this, { "Roguelike1WaitBattleStart" }).set_task_delay(0).set_retry_times(0).run();
}

bool asst::RoguelikeBattleTaskPlugin::get_stage_info()
{
    LogTraceFunction;

    wait_for_start();

    bool calced = false;

    if (m_stage_name.empty()) {
        const auto stage_name_task_ptr = Task.get("BattleStageName");
        sleep(stage_name_task_ptr->pre_delay);

        constexpr int StageNameRetryTimes = 50;
        for (int i = 0; i != StageNameRetryTimes; ++i) {
            cv::Mat image = m_ctrler->get_image();
            OcrWithPreprocessImageAnalyzer name_analyzer(image);

            name_analyzer.set_task_info(stage_name_task_ptr);
            if (!name_analyzer.analyze()) {
                continue;
            }

            for (const auto& tr : name_analyzer.get_result()) {
                auto side_info = Tile.calc(tr.text, true);
                if (side_info.empty()) {
                    continue;
                }
                m_stage_name = tr.text;
                m_side_tile_info = std::move(side_info);
                m_normal_tile_info = Tile.calc(m_stage_name, false);
                calced = true;
                break;
            }
            if (calced) {
                break;
            }
            std::this_thread::yield();
        }
    }
    else {
        m_side_tile_info = Tile.calc(m_stage_name, true);
        m_normal_tile_info = Tile.calc(m_stage_name, false);
        calced = true;
    }

    auto opt = RoguelikeCopilot.get_stage_data(m_stage_name);
    if (opt && !opt->replacement_home.empty()) {
        m_homes = opt->replacement_home;
        std::string log_str = "[ ";
        for (auto& home : m_homes) {
            if (!m_normal_tile_info.contains(home.location)) {
                Log.error("No replacement home point", home.location.x, home.location.y);
            }
            log_str += "( " + std::to_string(home.location.x) + ", " + std::to_string(home.location.y) + " ), ";
        }
        log_str += "]";
        Log.info("replacement home:", log_str);
        m_blacklist_location = opt->blacklist_location;
        m_stage_use_dice = opt->use_dice_stage;
        m_role_order = opt->role_order;
    }
    else {
        for (const auto& [loc, side] : m_normal_tile_info) {
            if (side.key == TilePack::TileKey::Home) {
                m_homes.emplace_back(ReplacementHome { loc, BattleDeployDirection::None });
            }
        }
        m_role_order = {
            BattleRole::Warrior, BattleRole::Pioneer, BattleRole::Medic,   BattleRole::Tank,  BattleRole::Sniper,
            BattleRole::Caster,  BattleRole::Support, BattleRole::Special, BattleRole::Drone,
        };
    }
    m_wait_melee.assign(m_homes.size(), true);
    m_wait_medic.assign(m_homes.size(), true);
    m_indeed_no_medic.assign(m_homes.size(), false);
    // 认为开局时未在所有路线放干员时为紧急状态，并把路线压入栈
    // 由于开局路线设为0，故0不需要被压入栈
    m_cur_home_index = 0;
    m_is_cur_urgent = true;
    for (size_t index = m_homes.size() - 1; index > 0; index--) {
        m_next_urgent_index.push(index);
    }
    m_index_count.assign(m_homes.size(), 0);
    if (opt && !opt->key_kills.empty()) {
        std::string log_str = "[ ";
        for (const auto& kills : opt->key_kills) {
            m_key_kills.emplace(kills);
            log_str += std::to_string(kills) + ", ";
        }
        log_str += "]";
        Log.info("key kills:", log_str);
    }

    if (m_homes.empty()) {
        Log.error("Unknown home pos");
    }

    if (calced) {
        auto cb_info = basic_info_with_what("StageInfo");
        auto& details = cb_info["details"];
        details["name"] = m_stage_name;
        details["size"] = m_side_tile_info.size();
        callback(AsstMsg::SubTaskExtraInfo, cb_info);
    }
    else {
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("StageInfoError"));
    }

    return calced;
}

bool asst::RoguelikeBattleTaskPlugin::battle_pause()
{
    return ProcessTask(*this, { "BattlePause" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::auto_battle()
{
    LogTraceFunction;

    // if (int hp = battle_analyzer.get_hp();
    //     hp != 0) {
    //     bool used_skills = false;
    //     if (hp < m_pre_hp) {    // 说明漏怪了，漏怪就开技能（
    //         for (const Rect& rect : battle_analyzer.get_ready_skills()) {
    //             used_skills = true;
    //             if (!use_skill(rect)) {
    //                 break;
    //             }
    //         }
    //     }
    //     m_pre_hp = hp;
    //     if (used_skills) {
    //         return true;
    //     }
    // }

    // 将存在于场上超过限时的召唤物所在的地块重新设为可用
    Time_Point now_time = std::chrono::system_clock::now();
    while ((!m_need_clear_tiles.empty()) && m_need_clear_tiles.top().placed_time < now_time) {
        const auto& placed_loc = m_need_clear_tiles.top().placed_loc;
        if (auto iter = m_used_tiles.find(placed_loc); iter != m_used_tiles.end()) {
            m_opers_in_field.erase((iter->second).first);
            m_used_tiles.erase(iter);
        }
        m_need_clear_tiles.pop();
    }

    const cv::Mat& image = m_ctrler->get_image();
    if (try_possible_skill(image)) {
        return true;
    }

    BattleImageAnalyzer battle_analyzer(image);
    battle_analyzer.set_target(BattleImageAnalyzer::Target::Roguelike);

    if (!battle_analyzer.analyze()) {
        return false;
    }

    battle_analyzer.sort_opers_by_cost();
    auto opers = battle_analyzer.get_opers();
    if (opers.empty()) {
        return true;
    }
    battle_analyzer.set_target(BattleImageAnalyzer::Target::Oper);

    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }

    size_t available_count = 0;
    size_t cooling_count = 0;
    std::vector<size_t> new_urgent;
    std::vector<std::string> cooling_opers;
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    bool has_dice = false;
    BattleRealTimeOper dice;
    for (auto& oper : opers) {
        if (oper.cooling) cooling_count++;
        if (oper.available) available_count++;
        if (oper.role == BattleRole::Drone) {
            if (!m_dice_image.empty()) {
                MatchImageAnalyzer dice_analyzer(image);
                dice_analyzer.set_templ(m_dice_image);
                if (dice_analyzer.analyze()) {
                    has_dice = true;
                    oper.name = "骰子";
                    dice = oper;
                }
            }
        }
    }
    bool can_use_dice = m_stage_use_dice && has_dice;
    if (!can_use_dice) {
        m_use_dice = false;
    }
    // 如果发现有新撤退的或者新转好cd的，就更新m_retreated_opers
    if (cooling_count != m_retreated_opers.size()) {
        const decltype(m_retreated_opers) ret_copy(m_retreated_opers);
        m_retreated_opers.clear();
        if (cooling_count > 0) {
            battle_pause();
        }
        Rect cur_rect;
        for (size_t i = 0; i < opers.size(); ++i) {
            const auto& cur_opers = battle_analyzer.get_opers();
            size_t offset = opers.size() > cur_opers.size() ? opers.size() - cur_opers.size() : 0;
            const auto& oper = cur_opers.at(i - offset);
            if (oper.cooling && oper.role != BattleRole::Drone) {
                cur_rect = oper.rect;
                m_ctrler->click(cur_rect);
                sleep(use_oper_task_ptr->pre_delay);
                const cv::Mat& new_image = m_ctrler->get_image();

                OcrWithPreprocessImageAnalyzer oper_name_analyzer(new_image);
                oper_name_analyzer.set_task_info("BattleOperName");
                oper_name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

                std::string oper_name = "Unknown";
                if (oper_name_analyzer.analyze()) {
                    oper_name_analyzer.sort_result_by_score();
                    oper_name = oper_name_analyzer.get_result().front().text;
                }

                battle_analyzer.set_image(new_image);
                battle_analyzer.analyze();
                battle_analyzer.sort_opers_by_cost();

                if (oper_name != "Unknown") {
                    m_retreated_opers.emplace(oper_name);
                    if (!ret_copy.contains(oper_name)) {
                        if (auto iter = m_opers_in_field.find(oper_name); iter != m_opers_in_field.end()) {
                            Log.info(oper_name, "retreated");
                            if (auto del_pos_melee = m_melee_for_home_index.find(iter->second);
                                del_pos_melee != m_melee_for_home_index.end()) {
                                m_wait_melee[del_pos_melee->second] = true;
                                new_urgent.emplace_back(del_pos_melee->second);
                                m_melee_for_home_index.erase(del_pos_melee);
                            }
                            else if (auto del_pos_medic = m_medic_for_home_index.find(iter->second);
                                     del_pos_medic != m_medic_for_home_index.end()) {
                                for (const size_t& home_index : del_pos_medic->second) {
                                    m_wait_medic[home_index] = true;
                                }
                                m_medic_for_home_index.erase(del_pos_medic);
                            }
                            m_used_tiles.erase(iter->second);
                            m_opers_in_field.erase(iter);
                        }
                    }
                }
            }
        }
        if (cooling_count > 0) {
            m_ctrler->click(cur_rect);
            battle_pause();
        }
    }
    if (new_urgent.empty()) {
        if ((!m_is_cur_urgent) || m_index_count[m_cur_home_index] == 0) {
            if (m_used_tiles.size() >= 2) {
                // 超过一半的人费用都没好，且没有紧急情况，那就不下人
                size_t not_cooling_count = opers.size() - cooling_count;
                if (available_count <= not_cooling_count / 2) {
                    Log.trace("already used", m_used_tiles.size(), ", now_total", opers.size(), ", available",
                              available_count, ", not_cooling", not_cooling_count);
                    return true;
                }
            }
        }
    }
    else {
        // 出现新的紧急情况，立即切到这条线路，并把其他紧急情况压入栈
        Log.info("New urgent situation detected");
        if (m_is_cur_urgent) {
            m_next_urgent_index.push(m_cur_home_index);
        }
        else {
            m_last_not_urgent = static_cast<int>(m_cur_home_index);
        }
        m_cur_home_index = new_urgent.at(0);
        for (size_t i = 1; i < new_urgent.size(); ++i) {
            m_next_urgent_index.push(new_urgent.at(i));
        }
        if (can_use_dice) {
            // 先放骰子，再放近战
            m_use_dice = true;
            m_next_urgent_index.push(new_urgent.at(0));
        }
    }

    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    // 点击当前最合适的干员
    BattleRealTimeOper opt_oper;
    bool oper_found = false;

    bool has_melee = false;
    bool has_medic = false;
    bool wait_melee = m_wait_melee[m_cur_home_index];
    bool wait_medic = m_wait_medic[m_cur_home_index];
    if (m_use_dice) {
        opt_oper = std::move(dice);
        oper_found = true;
        if (available_locations(opt_oper.role).empty()) {
            Log.info("Tiles full");
            return true;
        }
    }
    else {
        // 对于每个蓝门，先下个地面单位（如果有的话）
        // 第二个人下奶（如果有的话）
        for (const auto& op : opers) {
            if ((op.role == BattleRole::Warrior || op.role == BattleRole::Pioneer || op.role == BattleRole::Tank) &&
                (!op.cooling)) {
                has_melee = true;
            }
            else if (op.role == BattleRole::Medic && (!op.cooling)) {
                has_medic = true;
            }
        }

        wait_melee &= has_melee;
        wait_medic &= has_medic;
        for (auto role : m_role_order) {
            if (wait_melee) {
                if (role != BattleRole::Warrior && role != BattleRole::Pioneer && role != BattleRole::Tank) {
                    continue;
                }
            }
            else if (wait_medic) {
                if (role != BattleRole::Medic) {
                    continue;
                }
            }

            for (const auto& oper : opers) {
                if (!oper.available) {
                    continue;
                }
                if (oper.name == "骰子") {
                    continue;
                }
                if (oper.role == role) {
                    opt_oper = oper;
                    oper_found = true;
                    break;
                }
            }
            if (oper_found) {
                break;
            }
        }
        if (!oper_found) {
            return true;
        }

        // 预计算干员是否有点地方放
        if (available_locations(opt_oper.role).empty()) {
            Log.info("Tiles full");
            return true;
        }

        if (m_first_deploy) {
            m_first_deploy = false;
            battle_pause();
            Rect cur_rect;
            for (size_t i = 0; i < opers.size(); ++i) {
                const auto& cur_opers = battle_analyzer.get_opers();
                size_t offset = opers.size() > cur_opers.size() ? opers.size() - cur_opers.size() : 0;
                const auto& oper = cur_opers.at(i - offset);
                if (oper.role == BattleRole::Drone) {
                    cur_rect = oper.rect;
                    m_ctrler->click(cur_rect);
                    sleep(use_oper_task_ptr->pre_delay);
                    const cv::Mat& new_image = m_ctrler->get_image();

                    OcrWithPreprocessImageAnalyzer oper_name_analyzer(new_image);
                    oper_name_analyzer.set_task_info("BattleOperName");
                    oper_name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

                    battle_analyzer.set_image(new_image);
                    battle_analyzer.analyze();
                    battle_analyzer.sort_opers_by_cost();

                    if (!oper_name_analyzer.analyze()) continue;
                    oper_name_analyzer.sort_result_by_score();
                    if (oper_name_analyzer.get_result().front().text == "骰子") {
                        m_dice_image = opers.at(i).avatar;
                        break;
                    }
                }
            }
            if (!cur_rect.empty()) {
                m_ctrler->click(cur_rect);
            }
            battle_pause();
        }

        m_ctrler->click(opt_oper.rect);
        sleep(use_oper_task_ptr->pre_delay);

        OcrWithPreprocessImageAnalyzer oper_name_analyzer(m_ctrler->get_image());
        oper_name_analyzer.set_task_info("BattleOperName");
        oper_name_analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

        opt_oper.name = "Unknown";
        if (oper_name_analyzer.analyze()) {
            oper_name_analyzer.sort_result_by_score();
            opt_oper.name = oper_name_analyzer.get_result().front().text;
        }
    }

    // 计算最优部署位置及方向
    const auto& [placed_loc, direction] = calc_best_plan(opt_oper);

    // 将干员拖动到场上
    Point placed_point = m_side_tile_info.at(placed_loc).pos;
    Rect placed_rect(placed_point.x, placed_point.y, 1, 1);
    int dist = static_cast<int>(Point::distance(
        placed_point, { opt_oper.rect.x + opt_oper.rect.width / 2, opt_oper.rect.y + opt_oper.rect.height / 2 }));
    // 1000 是随便取的一个系数，把整数的 pre_delay 转成小数用的
    int duration = static_cast<int>(swipe_oper_task_ptr->pre_delay / 800.0 * dist * log10(dist));
    m_ctrler->swipe(opt_oper.rect, placed_rect, duration, true, 0);
    sleep(use_oper_task_ptr->rear_delay);

    // 将方向转换为实际的 swipe end 坐标点
    constexpr int coeff = 500;
    Point end_point = placed_point + (direction * coeff);

    m_ctrler->swipe(placed_point, end_point, swipe_oper_task_ptr->rear_delay, true, 100);

    if (opt_oper.role == BattleRole::Drone) {
        cancel_oper_selection();
        now_time = std::chrono::system_clock::now();
        if (opt_oper.name == "骰子") {
            m_need_clear_tiles.emplace(now_time + std::chrono::seconds(20), placed_loc);
        }
        else {
            m_need_clear_tiles.emplace(now_time + std::chrono::seconds(35), placed_loc);
        }
    }

    Log.info("Try to deploy oper", opt_oper.name);
    m_used_tiles.emplace(placed_loc, std::make_pair(opt_oper.name, opt_oper.role));
    m_opers_in_field.emplace(opt_oper.name, placed_loc);
    m_opers_used = true;
    if (wait_melee) {
        m_wait_melee[m_cur_home_index] = false;
        m_melee_for_home_index.emplace(placed_loc, m_cur_home_index);
    }
    else if (wait_medic) {
        // 两次轮到wait_medic的轮次时仍没有医疗干员奶当前位置，即认为奶不到
        if (m_wait_medic[m_cur_home_index]) {
            m_indeed_no_medic[m_cur_home_index] = false;
        }
        else {
            if (m_indeed_no_medic[m_cur_home_index]) {
                m_wait_medic[m_cur_home_index] = false;
            }
            else {
                m_indeed_no_medic[m_cur_home_index] = true;
            }
        }
    }
    // 第一次在这条路线上放近战干员时，应当保证有一半干员费用转好
    m_index_count[m_cur_home_index]++;
    // 如果有紧急需要部署的路线，那么下一条路线设为它
    if (m_next_urgent_index.empty()) {
        if (m_last_not_urgent != -1) {
            // 从上一次不是紧急的下一条路线开始
            m_cur_home_index = static_cast<size_t>(m_last_not_urgent) + 1;
            m_last_not_urgent = -1;
        }
        else {
            ++m_cur_home_index;
        }
        m_is_cur_urgent = false;
    }
    else {
        if (m_last_not_urgent == -1) {
            m_last_not_urgent = static_cast<int>(m_cur_home_index);
        }
        m_is_cur_urgent = true;
        m_cur_home_index = m_next_urgent_index.top();
        Log.info("Enter urgent situtation, to path", m_cur_home_index);
        m_next_urgent_index.pop();
    }

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::speed_up()
{
    return ProcessTask(*this, { "Roguelike1BattleSpeedUp" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::use_skill(const Rect& rect)
{
    m_ctrler->click(rect);
    sleep(Task.get("BattleUseOper")->pre_delay);

    ProcessTask task(*this, { "BattleUseSkillJustClick" });
    task.set_retry_times(0);
    return task.run();
}

bool asst::RoguelikeBattleTaskPlugin::retreat(const Point& point)
{
    m_ctrler->click(point);
    sleep(Task.get("BattleUseOper")->pre_delay);

    return ProcessTask(*this, { "BattleOperRetreatJustClick" }).run();
}

bool asst::RoguelikeBattleTaskPlugin::abandon()
{
    return ProcessTask(*this, { "Roguelike1BattleExitBegin" }).run();
}

void asst::RoguelikeBattleTaskPlugin::all_melee_retreat()
{
    for (const auto& loc : m_used_tiles | views::keys) {
        auto& tile_info = m_normal_tile_info[loc];
        auto& type = tile_info.buildable;
        if (type == Loc::Melee || type == Loc::All) {
            retreat(tile_info.pos);
        }
    }
}

void asst::RoguelikeBattleTaskPlugin::clear()
{
    m_opers_used = false;
    m_pre_hp = 0;
    m_homes.clear();
    m_wait_melee.clear();
    m_wait_medic.clear();
    m_indeed_no_medic.clear();
    m_melee_for_home_index.clear();
    m_medic_for_home_index.clear();
    decltype(m_next_urgent_index) empty_stack;
    m_next_urgent_index.swap(empty_stack);
    m_is_cur_urgent = false;
    m_last_not_urgent = -1;
    m_index_count.clear();
    m_blacklist_location.clear();
    m_retreated_opers.clear();
    decltype(m_need_clear_tiles) empty_heap;
    m_need_clear_tiles.swap(empty_heap);
    decltype(m_key_kills) empty_queue;
    m_key_kills.swap(empty_queue);
    m_cur_home_index = 0;
    m_stage_name.clear();
    m_side_tile_info.clear();
    m_used_tiles.clear();
    m_opers_in_field.clear();
    m_kills = 0;
    m_total_kills = 0;
    m_use_dice = false;
    m_stage_use_dice = true;
    m_dice_image = cv::Mat();
    m_first_deploy = true;

    for (auto& [key, status] : m_restore_status) {
        m_status->set_number(key, status);
    }
    m_restore_status.clear();
}

bool asst::RoguelikeBattleTaskPlugin::try_possible_skill(const cv::Mat& image)
{
    if (!check_key_kills(image)) {
        return false;
    }

    auto task_ptr = Task.get("BattleAutoSkillFlag");
    const Rect& skill_roi_move = task_ptr->rect_move;

    MatchImageAnalyzer analyzer(image);
    analyzer.set_task_info(task_ptr);
    bool used = false;
    for (auto& [loc, oper_pair] : m_used_tiles) {
        std::string status_key = "RoguelikeSkillUsage-" + oper_pair.first;
        auto usage = BattleSkillUsage::Possibly;
        auto usage_opt = m_status->get_number(status_key);
        if (usage_opt) {
            usage = static_cast<BattleSkillUsage>(usage_opt.value());
        }

        if (usage != BattleSkillUsage::Possibly && usage != BattleSkillUsage::Once) {
            continue;
        }
        const Point pos = m_normal_tile_info.at(loc).pos;
        const Rect pos_rect(pos.x, pos.y, 1, 1);
        const Rect roi = pos_rect.move(skill_roi_move);
        analyzer.set_roi(roi);
        if (!analyzer.analyze()) {
            continue;
        }
        m_ctrler->click(pos_rect);
        sleep(Task.get("BattleUseOper")->pre_delay);
        bool ret = ProcessTask(*this, { "BattleSkillReadyOnClick" }).set_retry_times(2).run();
        if (!ret) {
            cancel_oper_selection();
        }
        used |= ret;
        if (usage == BattleSkillUsage::Once) {
            m_status->set_number(status_key, static_cast<int64_t>(BattleSkillUsage::OnceUsed));
            m_restore_status[status_key] = static_cast<int64_t>(BattleSkillUsage::Once);
        }
    }
    return used;
}

bool asst::RoguelikeBattleTaskPlugin::check_key_kills(const cv::Mat& image)
{
    if (m_key_kills.empty()) {
        return true;
    }
    int need_kills = m_key_kills.front();

    BattleImageAnalyzer analyzer(image);
    if (m_total_kills) {
        analyzer.set_pre_total_kills(m_total_kills);
    }
    analyzer.set_target(BattleImageAnalyzer::Target::Kills);
    if (analyzer.analyze()) {
        m_kills = analyzer.get_kills();
        m_total_kills = analyzer.get_total_kills();
        if (m_kills >= need_kills) {
            m_key_kills.pop();
            return true;
        }
    }
    return false;
}

bool asst::RoguelikeBattleTaskPlugin::wait_start()
{
    auto start_time = std::chrono::system_clock::now();
    auto check_time = [&]() -> bool {
        using namespace std::chrono_literals;
        return std::chrono::system_clock::now() - start_time > 1min;
    };

    MatchImageAnalyzer officially_begin_analyzer;
    officially_begin_analyzer.set_task_info("BattleOfficiallyBegin");
    cv::Mat image;
    while (!need_exit() && !check_time()) {
        image = m_ctrler->get_image();
        officially_begin_analyzer.set_image(image);
        if (officially_begin_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    BattleImageAnalyzer oper_analyzer;
    oper_analyzer.set_target(BattleImageAnalyzer::Target::Oper);
    while (!need_exit() && !check_time()) {
        image = m_ctrler->get_image();
        oper_analyzer.set_image(image);
        if (oper_analyzer.analyze()) {
            break;
        }
        std::this_thread::yield();
    }

    std::filesystem::create_directory("map");
    for (const auto& [loc, info] : m_normal_tile_info) {
        std::string text = "( " + std::to_string(loc.x) + ", " + std::to_string(loc.y) + " )";
        cv::putText(image, text, cv::Point(info.pos.x - 30, info.pos.y), 1, 1.2, cv::Scalar(0, 0, 255), 2);
    }

    // 识别一帧总击杀数
    BattleImageAnalyzer kills_analyzer(image);
    kills_analyzer.set_target(BattleImageAnalyzer::Target::Kills);
    if (kills_analyzer.analyze()) {
        m_kills = kills_analyzer.get_kills();
        m_total_kills = kills_analyzer.get_total_kills();
    }

    asst::imwrite("map/" + m_stage_name + ".png", image);
    return true;
}

bool asst::RoguelikeBattleTaskPlugin::cancel_oper_selection()
{
    return ProcessTask(*this, { "BattleCancelSelection" }).run();
}

std::vector<asst::Point> asst::RoguelikeBattleTaskPlugin::available_locations(BattleRole role)
{
    LogTraceFunction;

    auto buildable_type = Loc::All;
    switch (role) {
    case BattleRole::Medic:
    case BattleRole::Support:
    case BattleRole::Sniper:
    case BattleRole::Caster:
        buildable_type = Loc::Ranged;
        break;
    case BattleRole::Pioneer:
    case BattleRole::Warrior:
    case BattleRole::Tank:
        buildable_type = Loc::Melee;
        break;
    case BattleRole::Special:
    case BattleRole::Drone:
    default:
        //// 特种和无人机，有的只能放地面，有的又只能放高台，不好判断
        //// 笨办法，都试试，总有一次能成的
        //{
        //    static Loc static_loc = Loc::Melee;
        //    loc = static_loc;
        //    if (static_loc == Loc::Melee) {
        //        static_loc = Loc::Ranged;
        //    }
        //    else {
        //        static_loc = Loc::Melee;
        //    }
        //}
        // 大部分无人机都是可以摆在地上的，摆烂了
        buildable_type = Loc::Melee;
        break;
    }

    std::vector<Point> result;
    for (const auto& [loc, tile] : m_normal_tile_info) {
        if ((tile.buildable == buildable_type || tile.buildable == Loc::All) &&
            tile.key != TilePack::TileKey::DeepSea && // 水上要先放板子才能放人，肉鸽里也没板子，那就当作不可放置
            !m_used_tiles.contains(loc) && !m_blacklist_location.contains(loc)) {
            result.emplace_back(loc);
        }
    }
    return result;
}

asst::BattleAttackRange asst::RoguelikeBattleTaskPlugin::get_attack_range(const BattleRealTimeOper& oper)
{
    int64_t elite = m_status->get_number(RuntimeStatus::RoguelikeCharElitePrefix + oper.name).value_or(0);
    BattleAttackRange right_attack_range = BattleData.get_range(oper.name, elite);

    if (right_attack_range == BattleDataConfiger::EmptyRange) {
        switch (oper.role) {
        case BattleRole::Support:
            right_attack_range = {
                Point(-1, -1), Point(0, -1), Point(1, -1), Point(2, -1), //
                Point(-1, 0),  Point(0, 0),  Point(1, 0),  Point(2, 0),  //
                Point(-1, 1),  Point(0, 1),  Point(1, 1),  Point(2, 1),  //
            };
            break;
        case BattleRole::Caster:
            right_attack_range = {
                Point(0, -1), Point(1, -1), Point(2, -1),              //
                Point(0, 0),  Point(1, 0),  Point(2, 0),  Point(3, 0), //
                Point(0, 1),  Point(1, 1),  Point(2, 1),               //
            };
            break;
        case BattleRole::Medic:
        case BattleRole::Sniper:
            right_attack_range = {
                Point(0, -1), Point(1, -1), Point(2, -1), Point(3, -1), //
                Point(0, 0),  Point(1, 0),  Point(2, 0),  Point(3, 0),  //
                Point(0, 1),  Point(1, 1),  Point(2, 1),  Point(3, 1),  //
            };
            break;
        case BattleRole::Warrior:
            right_attack_range = { Point(0, 0), Point(1, 0), Point(2, 0) };
            break;
        case BattleRole::Special:
        case BattleRole::Tank:
        case BattleRole::Pioneer:
        case BattleRole::Drone:
            right_attack_range = { Point(0, 0), Point(1, 0) };
            break;
        default:
            break;
        }
    }

    return right_attack_range;
}

asst::RoguelikeBattleTaskPlugin::DeployInfo asst::RoguelikeBattleTaskPlugin::calc_best_plan(
    const BattleRealTimeOper& oper)
{
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }

    Point home(5, 5); // 实在找不到家门了，随便取个点当家门用算了，一般是地图的中间
    Point recommended_direction;
    if (m_cur_home_index < m_homes.size()) {
        const auto& rp_home = m_homes.at(m_cur_home_index);
        home = rp_home.location;
        static const std::unordered_map<BattleDeployDirection, Point> direction_map = {
            { BattleDeployDirection::Up, Point::up() },     { BattleDeployDirection::Down, Point::down() },
            { BattleDeployDirection::Left, Point::left() }, { BattleDeployDirection::Right, Point::right() },
            { BattleDeployDirection::None, Point() },
        };
        recommended_direction = direction_map.at(rp_home.direction);
    }

    auto comp_dist = [&](const Point& lhs, const Point& rhs) -> bool {
        int lhs_y_dist = std::abs(lhs.y - home.y);
        int lhs_dist = std::abs(lhs.x - home.x) + lhs_y_dist;
        int rhs_y_dist = std::abs(rhs.y - home.y);
        int rhs_dist = std::abs(rhs.x - home.x) + rhs_y_dist;
        // 距离一样选择 x 轴上的，因为一般的地图都是横向的长方向
        return lhs_dist == rhs_dist ? lhs_y_dist < rhs_y_dist : lhs_dist < rhs_dist;
    };

    std::vector<Point> available_loc = available_locations(oper.role);

    // 把所有可用的点按距离排个序
    ranges::sort(available_loc, comp_dist);

    if (oper.name == "骰子") {
        return { available_loc.back(), Point::right() };
    }

    // 取距离最近的N个点，计算分数。然后使用得分最高的点
    constexpr int CalcPointCount = 4;
    if (available_loc.size() > CalcPointCount) {
        available_loc.erase(available_loc.begin() + CalcPointCount, available_loc.end());
    }

    Point best_location;
    Point best_direction;
    int max_score = INT_MIN;

    const auto& near_loc = available_loc.at(0);
    int min_dist = std::abs(near_loc.x - home.x) + std::abs(near_loc.y - home.y);

    for (const auto& loc : available_loc) {
        auto cur_result = calc_best_direction_and_score(loc, oper, recommended_direction);
        // 离得远的要扣分
        constexpr int DistWeights = -1050;
        int extra_dist = std::abs(loc.x - home.x) + std::abs(loc.y - home.y) - min_dist;
        int extra_dist_score = DistWeights * extra_dist;
        if (oper.role == BattleRole::Medic) { // 医疗干员离得远无所谓
            extra_dist_score = 0;
        }

        if (cur_result.second + extra_dist_score > max_score) {
            max_score = cur_result.second + extra_dist_score;
            best_location = loc;
            best_direction = cur_result.first;
        }
    }

    // 如果是医疗干员，判断覆盖范围内有无第一次放置的干员
    if (oper.role == BattleRole::Medic) {
        BattleAttackRange right_attack_range = get_attack_range(oper);
        for (const Point& direction : { Point::right(), Point::up(), Point::left(), Point::down() }) {
            if (direction == best_direction) break;
            for (Point& point : right_attack_range)
                point = { point.y, -point.x };
        }
        std::vector<size_t> contain_index;
        for (const Point& relative_pos : right_attack_range) {
            Point absolute_pos = best_location + relative_pos;
            if (auto iter = m_melee_for_home_index.find(absolute_pos); iter != m_melee_for_home_index.end()) {
                m_wait_medic[iter->second] = false;
                contain_index.emplace_back(iter->second);
            }
        }
        if (!contain_index.empty()) {
            m_medic_for_home_index.emplace(best_location, std::move(contain_index));
        }
    }

    return { best_location, best_direction };
}

std::pair<asst::Point, int> asst::RoguelikeBattleTaskPlugin::calc_best_direction_and_score(
    Point loc, const BattleRealTimeOper& oper, Point recommended_direction)
{
    LogTraceFunction;

    // 根据家门的方向计算一下大概的朝向
    if (m_cur_home_index >= m_homes.size()) {
        m_cur_home_index = 0;
    }
    Point home_loc(5, 5);
    if (m_cur_home_index < m_homes.size()) {
        home_loc = m_homes.at(m_cur_home_index).location;
    }

    auto sgn = [](const int& x) -> int {
        if (x > 0) return 1;
        if (x < 0) return -1;
        return 0;
    };

    Point base_direction(0, 0);
    if (loc.x == home_loc.x) {
        base_direction.y = sgn(loc.y - home_loc.y);
    }
    else {
        base_direction.x = sgn(loc.x - home_loc.x);
    }
    Point home_direction(-base_direction.x, -base_direction.y);
    // 医疗反着算
    if (oper.role == BattleRole::Medic) {
        base_direction = -base_direction;
    }

    // 按朝右算，后面根据方向做转换
    BattleAttackRange right_attack_range = get_attack_range(oper);

    int max_score = 0;
    Point opt_direction;

    for (const Point& direction : { Point::right(), Point::up(), Point::left(), Point::down() }) {
        int score = 0;
        for (const Point& relative_pos : right_attack_range) {
            Point absolute_pos = loc + relative_pos;

            using TileKey = TilePack::TileKey;
            // 战斗干员朝向的权重
            static const std::unordered_map<TileKey, int> TileKeyFightWeights = {
                { TileKey::Invalid, 0 },    { TileKey::Forbidden, 0 },  { TileKey::Wall, 500 },
                { TileKey::Road, 1000 },    { TileKey::Home, 500 },     { TileKey::EnemyHome, 1000 },
                { TileKey::Airport, 1000 }, { TileKey::Floor, 1000 },   { TileKey::Hole, 0 },
                { TileKey::Telin, 700 },    { TileKey::Telout, 800 },   { TileKey::Grass, 500 },
                { TileKey::DeepSea, 1000 }, { TileKey::Volcano, 1000 }, { TileKey::Healing, 1000 },
                { TileKey::Fence, 800 },
            };
            // 治疗干员朝向的权重
            static const std::unordered_map<TileKey, int> TileKeyMedicWeights = {
                { TileKey::Invalid, 0 },  { TileKey::Forbidden, 0 },  { TileKey::Wall, 1000 },
                { TileKey::Road, 1000 },  { TileKey::Home, 0 },       { TileKey::EnemyHome, 0 },
                { TileKey::Airport, 0 },  { TileKey::Floor, 0 },      { TileKey::Hole, 0 },
                { TileKey::Telin, 0 },    { TileKey::Telout, 0 },     { TileKey::Grass, 500 },
                { TileKey::DeepSea, 0 },  { TileKey::Volcano, 1000 }, { TileKey::Healing, 1000 },
                { TileKey::Fence, 1000 },
            };

            switch (oper.role) {
            case BattleRole::Medic:
                if (m_used_tiles.contains(absolute_pos) &&
                    m_used_tiles[absolute_pos].second != BattleRole::Drone) // 根据哪个方向上人多决定朝向哪
                    score += 10000;
                if (auto iter = m_side_tile_info.find(absolute_pos); iter != m_side_tile_info.end())
                    score += TileKeyMedicWeights.at(iter->second.key);
                break;
            default:
                if (auto iter = m_side_tile_info.find(absolute_pos); iter != m_side_tile_info.end())
                    score += TileKeyFightWeights.at(iter->second.key);
                break;
            }
        }

        if (direction == base_direction) {
            score += 300;
        }

        if (oper.role != BattleRole::Medic && direction == home_direction) {
            score -= 500;
        }

        if (oper.role != BattleRole::Medic && direction == recommended_direction) {
            score += 2000;
        }

        if (score > max_score) {
            max_score = score;
            opt_direction = direction;
        }

        // rotate relative attack range counterclockwise
        for (Point& point : right_attack_range)
            point = { point.y, -point.x };
    }

    return std::make_pair(opt_direction, max_score);
}
